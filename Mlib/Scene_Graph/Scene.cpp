#include "Scene.hpp"
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Geometry/Mesh/Colored_Vertex_Array.hpp>
#include <Mlib/Log.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Scene_Graph/Aggregate_Renderer.hpp>
#include <Mlib/Scene_Graph/Scene_Graph_Config.hpp>

using namespace Mlib;

Scene::Scene(
    AggregateRenderer* small_sorted_aggregate_renderer,
    AggregateRenderer* large_aggregate_renderer)
: small_sorted_aggregate_renderer_{small_sorted_aggregate_renderer},
  large_aggregate_renderer_{large_aggregate_renderer},
  large_aggregate_renderer_initialized_{false},
  uuid_{0}
{}

void Scene::add_root_node(
    const std::string& name,
    SceneNode* scene_node)
{
    LOG_FUNCTION("Scene::add_root_node");
    std::unique_lock lock{dynamic_mutex_};
    LOG_INFO("Lock acquired");
    register_node(name, scene_node);
    root_nodes_.insert(std::make_pair(name, scene_node));
}

void Scene::add_static_root_node(
    const std::string& name,
    SceneNode* scene_node)
{
    std::unique_lock lock{static_mutex_};
    register_node(name, scene_node);
    static_root_nodes_.insert(std::make_pair(name, scene_node));
}

void Scene::add_root_aggregate_node(
    const std::string& name,
    SceneNode* scene_node)
{
    std::unique_lock lock{aggregate_mutex_};
    register_node(name, scene_node);
    root_aggregate_nodes_.insert(std::make_pair(name, scene_node));
}

void Scene::delete_root_node(const std::string& name) {
    LOG_FUNCTION("Scene::delete_root_node");
    std::unique_lock lock{dynamic_mutex_};
    LOG_INFO("Lock acquired");
    auto it = root_nodes_.find(name);
    if (it == root_nodes_.end()) {
        throw std::runtime_error("Could not find root node with name " + name);
    }
    root_nodes_.erase(it);
    unregister_node(name);
}

void Scene::delete_root_nodes(const std::regex& regex) {
    LOG_FUNCTION("Scene::delete_root_nodes");
    std::unique_lock lock{dynamic_mutex_};
    LOG_INFO("Lock acquired");
    for(auto it = root_nodes_.begin(); it != root_nodes_.end(); ) {
        auto n = it++;
        if (std::regex_match(n->first, regex)) {
            root_nodes_.erase(n->first);
        }
    }
    unregister_nodes(regex);
}

Scene::~Scene() {
    if (aggregation_thread_.joinable()) {
        aggregation_thread_.join();
    }
}

bool Scene::contains_node(const std::string& name) const {
    return nodes_.contains(name);
}

void Scene::register_node(
    const std::string& name,
    SceneNode* scene_node)
{
    std::unique_lock lock{registration_mutex_};
    if (nodes_.find(name) != nodes_.end()) {
        throw std::runtime_error("Scene node with name \"" + name + "\" already exists");
    }
    nodes_.insert(std::make_pair(name, scene_node));
}

void Scene::unregister_node(const std::string& name) {
    std::unique_lock lock{registration_mutex_};
    auto it = nodes_.find(name);
    if (it == nodes_.end()) {
        throw std::runtime_error("Could not find node with name \"" + name + '"');
    }
    nodes_.erase(it);
}

void Scene::unregister_nodes(const std::regex& regex) {
    std::unique_lock lock{registration_mutex_};
    for(auto it = nodes_.begin(); it != nodes_.end(); ) {
        auto n = *it++;
        if (std::regex_match(n.first, regex)) {
            nodes_.erase(n.first);
        }
    }
}

SceneNode* Scene::get_node(const std::string& name) const {
    // necessary if "delete_node" is called
    std::shared_lock dynamic_lock{dynamic_mutex_};
    // necessary if "unregister_node(s)" is called
    std::shared_lock registration_lock{registration_mutex_};
    auto it = nodes_.find(name);
    if (it == nodes_.end()) {
        throw std::runtime_error("Could not find node with name \"" + name + '"');
    }
    return it->second;
}

void Scene::render(
    const FixedArray<float, 4, 4>& vp,
    const FixedArray<float, 4, 4>& iv,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    ExternalRenderPass external_render_pass) const
{
    std::list<std::pair<FixedArray<float, 4, 4>, Light*>> lights;
    std::list<Blended> blended;
    if ((external_render_pass.pass == ExternalRenderPass::LIGHTMAP_TO_TEXTURE) && !external_render_pass.black_node_name.empty()) {
        std::shared_lock lock{dynamic_mutex_};
        root_nodes_.at(external_render_pass.black_node_name)->render(vp, fixed_identity_array<float, 4>(), iv, lights, blended, render_config, scene_graph_config, external_render_pass);
    } else {
        // |         |Lights|Blended|Large|Small|Move|
        // |---------|------|-------|-----|-----|----|
        // |Dynamic  |x     |x      |     |     |x   |
        // |Static   |x     |x      |x    |x    |    |
        // |Aggregate|      |       |x    |x    |    |
        LOG_FUNCTION("Scene::render lights");
        {
            std::shared_lock lock{dynamic_mutex_};
            for(const auto& n : root_nodes_) {
                n.second->append_lights_to_queue(fixed_identity_array<float, 4>(), lights);
            }
        }
        {
            std::shared_lock lock{static_mutex_};
            for(const auto& n : static_root_nodes_) {
                n.second->append_lights_to_queue(fixed_identity_array<float, 4>(), lights);
            }
        }
        LOG_INFO("Scene::render non-blended");
        {
            {
                std::shared_lock lock{dynamic_mutex_};
                for(const auto& n : root_nodes_) {
                    n.second->render(vp, fixed_identity_array<float, 4>(), iv, lights, blended, render_config, scene_graph_config, external_render_pass);
                }
            }
            {
                std::shared_lock lock{static_mutex_};
                for(const auto& n : static_root_nodes_) {
                    n.second->render(vp, fixed_identity_array<float, 4>(), iv, lights, blended, render_config, scene_graph_config, external_render_pass);
                }
            }
        }
        LOG_INFO("Scene::render large_aggregate_renderer");
        if (large_aggregate_renderer_ != nullptr) {
            if (!large_aggregate_renderer_initialized_) {
                std::list<std::shared_ptr<ColoredVertexArray>> aggregate_queue;
                {
                    LOG_INFO("Scene::render large_aggregate_renderer static_mutex (0)");
                    std::shared_lock lock{static_mutex_};
                    LOG_INFO("Scene::render large_aggregate_renderer static_mutex (1)");
                    for(const auto& n : static_root_nodes_) {
                        n.second->append_large_aggregates_to_queue(fixed_identity_array<float, 4>(), aggregate_queue, scene_graph_config);
                    }
                }
                {
                    LOG_INFO("Scene::render large_aggregate_renderer aggregate_mutex (0)");
                    std::shared_lock lock{aggregate_mutex_};
                    LOG_INFO("Scene::render large_aggregate_renderer aggregate_mutex (1)");
                    for(const auto& n : root_aggregate_nodes_) {
                        n.second->append_large_aggregates_to_queue(fixed_identity_array<float, 4>(), aggregate_queue, scene_graph_config);
                    }
                }
                large_aggregate_renderer_->update_aggregates(aggregate_queue);
                large_aggregate_renderer_initialized_ = true;
            }
            large_aggregate_renderer_->render_aggregates(vp, iv, lights, scene_graph_config, render_config, external_render_pass);
        }
        // Contains continuous alpha and must therefore be rendered late.
        LOG_INFO("Scene::render small_sorted_aggregate_renderer");
        if ((small_sorted_aggregate_renderer_ != nullptr) &&
            (external_render_pass.pass != ExternalRenderPass::LIGHTMAP_TO_TEXTURE) &&
            (external_render_pass.pass != ExternalRenderPass::DIRTMAP)) {
            aggregate_small_i_ = (aggregate_small_i_ + 1) % scene_graph_config.aggregate_update_interval;
            if (aggregate_small_i_ == 0) {
                if (!aggregation_thread_.joinable()) {
                    // copy "vp", "scene_graph_config" and "external_render_pass"
                    aggregation_thread_ = std::thread{[this, vp, scene_graph_config, external_render_pass](){
                        std::list<std::pair<float, std::shared_ptr<ColoredVertexArray>>> aggregate_queue;
                        {
                            std::shared_lock lock{static_mutex_};
                            for(const auto& n : static_root_nodes_) {
                                n.second->append_sorted_aggregates_to_queue(vp, fixed_identity_array<float, 4>(), aggregate_queue, scene_graph_config);
                            }
                        }
                        {
                            std::shared_lock lock{aggregate_mutex_};
                            for(const auto& n : root_aggregate_nodes_) {
                                n.second->append_sorted_aggregates_to_queue(vp, fixed_identity_array<float, 4>(), aggregate_queue, scene_graph_config);
                            }
                        }
                        aggregate_queue.sort([](auto& a, auto& b){ return a.first < b.first; });
                        std::list<std::shared_ptr<ColoredVertexArray>> sorted_aggregate_queue;
                        for(auto& e : aggregate_queue) {
                            sorted_aggregate_queue.push_back(std::move(e.second));
                        }
                        small_sorted_aggregate_renderer_->update_aggregates(sorted_aggregate_queue);
                        aggregate_small_sorted_done_ = true;
                    }};
                }
            } else {
                if (aggregate_small_sorted_done_) {
                    aggregation_thread_.join();
                    aggregate_small_sorted_done_ = false;
                }
            }
            small_sorted_aggregate_renderer_->render_aggregates(vp, iv, lights, scene_graph_config, render_config, external_render_pass);
        }
    }
    // Contains continuous alpha and must therefore be rendered late.
    LOG_INFO("Scene::render blended");
    blended.sort([](Blended& a, Blended& b){ return a.mvp(2, 3) > b.mvp(2, 3); });
    for(const auto& b : blended) {
        b.renderable->render(b.mvp, b.m, iv, lights, scene_graph_config, render_config, {external_render_pass, InternalRenderPass::BLENDED});
    }
}

void Scene::move() {
    LOG_FUNCTION("Scene::move");
    std::unique_lock lock{dynamic_mutex_};
    LOG_INFO("Lock acquired");
    for(const auto& n : root_nodes_) {
        n.second->move(fixed_identity_array<float, 4>());
    }
}

size_t Scene::get_uuid() {
    std::lock_guard guard{uuid_mutex_};
    return uuid_++;
}

void Scene::print() const {
    std::string rec(1, '-');
    std::cout << "Dynamic nodes" << std::endl;
    for(const auto& x : root_nodes_) {
        std::cout << rec << " " << x.first << std::endl;
        x.second->print(2);
    }
    std::cout << "Static nodes" << std::endl;
    for(const auto& x : static_root_nodes_) {
        std::cout << rec << " " << x.first << std::endl;
        x.second->print(2);
    }
    std::cout << "Aggregate nodes" << std::endl;
    for(const auto& x : root_aggregate_nodes_) {
        std::cout << rec << " " << x.first << std::endl;
        x.second->print(2);
    }
}
