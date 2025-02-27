#include "Scene.hpp"
#include <Mlib/Array/Chunked_Array.hpp>
#include <Mlib/Geometry/Coordinates/Homogeneous.hpp>
#include <Mlib/Geometry/Instance/Rendering_Dynamics.hpp>
#include <Mlib/Geometry/Mesh/Colored_Vertex_Array.hpp>
#include <Mlib/Geometry/Mesh/Transformed_Colored_Vertex_Array.hpp>
#include <Mlib/Log.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Memory/Recursive_Deletion.hpp>
#include <Mlib/Scene_Graph/Batch_Renderers/IAggregate_Renderer.hpp>
#include <Mlib/Scene_Graph/Batch_Renderers/IInstances_Renderer.hpp>
#include <Mlib/Scene_Graph/Batch_Renderers/Task_Location.hpp>
#include <Mlib/Scene_Graph/Containers/Root_Nodes.hpp>
#include <Mlib/Scene_Graph/Delete_Node_Mutex.hpp>
#include <Mlib/Scene_Graph/Elements/Blended.hpp>
#include <Mlib/Scene_Graph/Elements/Color_Style.hpp>
#include <Mlib/Scene_Graph/Elements/Dynamic_Style.hpp>
#include <Mlib/Scene_Graph/Elements/Light.hpp>
#include <Mlib/Scene_Graph/Elements/Renderable.hpp>
#include <Mlib/Scene_Graph/Elements/Rendering_Strategies.hpp>
#include <Mlib/Scene_Graph/Elements/Scene_Node.hpp>
#include <Mlib/Scene_Graph/Instances/Large_Instances_Queue.hpp>
#include <Mlib/Scene_Graph/Instances/Small_Instances_Queues.hpp>
#include <Mlib/Scene_Graph/Interfaces/IDynamic_Lights.hpp>
#include <Mlib/Scene_Graph/Interfaces/IParticle_Renderer.hpp>
#include <Mlib/Scene_Graph/Interfaces/ITrail_Renderer.hpp>
#include <Mlib/Scene_Graph/Interfaces/Particle_Substrate.hpp>
#include <Mlib/Scene_Graph/Render_Pass_Extended.hpp>
#include <Mlib/Scene_Graph/Resources/Scene_Node_Resources.hpp>
#include <Mlib/Scene_Graph/Scene_Graph_Config.hpp>
#include <Mlib/Threads/Unlock_Guard.hpp>
#include <Mlib/Throw_Or_Abort.hpp>
#include <Mlib/Time/Fps/Lag_Finder.hpp>
#include <mutex>

using namespace Mlib;

using NodeRawPtrs = ChunkedArray<std::list<std::vector<const SceneNode*>>>;
using NodeDanglingPtrs = ChunkedArray<std::list<std::vector<DanglingPtr<const SceneNode>>>>;
static const size_t CHUNK_SIZE = 1000;

Scene::Scene(
    std::string name,
    DeleteNodeMutex& delete_node_mutex,
    SceneNodeResources* scene_node_resources,
    IParticleRenderer* particle_renderer,
    ITrailRenderer* trail_renderer,
    IDynamicLights* dynamic_lights)
    : delete_node_mutex_{ delete_node_mutex }
    , morn_{ *this }
    , root_nodes_{ morn_.create("root_nodes") }
    , static_root_nodes_{ morn_.create("static_root_nodes") }
    , root_aggregate_once_nodes_{ morn_.create("root_aggregate_once_nodes") }
    , root_aggregate_always_nodes_{ morn_.create("root_aggregate_always_nodes") }
    , root_instances_once_nodes_{ morn_.create("root_instances_once_nodes") }
    , root_instances_always_nodes_{ morn_.create("root_instances_always_nodes") }
    , name_{ std::move(name) }
    , large_aggregate_bg_worker_{ "Large_agg_BG" }
    , large_instances_bg_worker_{ "Large_inst_BG" }
    , small_aggregate_bg_worker_{ "Small_agg_BG" }
    , small_instances_bg_worker_{ "Small_inst_BG" }
    , uuid_{ 0 }
    , shutting_down_{ false }
    , scene_node_resources_{ scene_node_resources }
    , particle_renderer_{ particle_renderer }
    , trail_renderer_{ trail_renderer }
    , dynamic_lights_{ dynamic_lights }
    , ncleanups_required_{ 0 }
{}

void Scene::add_moving_root_node(
    const std::string& name,
    DanglingUniquePtr<SceneNode>&& scene_node)
{
    std::scoped_lock lock{ mutex_ };
    LOG_FUNCTION("Scene::add_root_node");
    root_nodes_.add_root_node(name, std::move(scene_node), SceneNodeState::DYNAMIC);
}

void Scene::add_static_root_node(
    const std::string& name,
    DanglingUniquePtr<SceneNode>&& scene_node)
{
    std::scoped_lock lock{ mutex_ };
    static_root_nodes_.add_root_node(name, std::move(scene_node), SceneNodeState::STATIC);
}

void Scene::add_root_aggregate_once_node(
    const std::string& name,
    DanglingUniquePtr<SceneNode>&& scene_node)
{
    std::scoped_lock lock{ mutex_ };
    root_aggregate_once_nodes_.add_root_node(name, std::move(scene_node), SceneNodeState::STATIC);
}

void Scene::add_root_aggregate_always_node(
    const std::string& name,
    DanglingUniquePtr<SceneNode>&& scene_node)
{
    std::scoped_lock lock{ mutex_ };
    root_aggregate_always_nodes_.add_root_node(name, std::move(scene_node), SceneNodeState::STATIC);
}

void Scene::add_root_instances_once_node(
    const std::string& name,
    DanglingUniquePtr<SceneNode>&& scene_node)
{
    std::scoped_lock lock{ mutex_ };
    root_instances_once_nodes_.add_root_node(name, std::move(scene_node), SceneNodeState::STATIC);
}

void Scene::add_root_instances_always_node(
    const std::string& name,
    DanglingUniquePtr<SceneNode>&& scene_node)
{
    std::scoped_lock lock{ mutex_ };
    root_instances_always_nodes_.add_root_node(name, std::move(scene_node), SceneNodeState::STATIC);
}

void Scene::auto_add_root_node(
    const std::string& name,
    DanglingUniquePtr<SceneNode>&& scene_node,
    RenderingDynamics rendering_dynamics)
{
    add_root_node(
        name,
        std::move(scene_node),
        rendering_dynamics,
        scene_node->rendering_strategies());
}

void Scene::add_root_node(
    const std::string& name,
    DanglingUniquePtr<SceneNode>&& scene_node,
    RenderingDynamics rendering_dynamics,
    RenderingStrategies rendering_strategy)
{
    switch (rendering_strategy) {
    case RenderingStrategies::NONE:
        THROW_OR_ABORT("Rendering strategy of node \"" + name + "\" is \"none\"");
    case RenderingStrategies::OBJECT:
        switch (rendering_dynamics) {
        case RenderingDynamics::STATIC:
            add_static_root_node(name, std::move(scene_node));
            return;
        case RenderingDynamics::MOVING:
            add_moving_root_node(name, std::move(scene_node));
            return;
        }
        THROW_OR_ABORT(
            "Unknown rendering dynamics: " + std::to_string((int)rendering_dynamics));
    case RenderingStrategies::MESH_ONCE:
        if (rendering_dynamics != RenderingDynamics::STATIC) {
            THROW_OR_ABORT("Mesh aggregation requires static rendering dynamics");
        }
        add_root_aggregate_once_node(name, std::move(scene_node));
        return;
    case RenderingStrategies::MESH_SORTED_CONTINUOUSLY:
        if (rendering_dynamics != RenderingDynamics::STATIC) {
            THROW_OR_ABORT("Mesh aggregation requires static rendering dynamics");
        }
        add_root_aggregate_always_node(name, std::move(scene_node));
        return;
    case RenderingStrategies::INSTANCES_ONCE:
        if (rendering_dynamics != RenderingDynamics::STATIC) {
            THROW_OR_ABORT("Instances require static rendering dynamics");
        }
        add_root_instances_once_node(name, std::move(scene_node));
        return;
    case RenderingStrategies::INSTANCES_SORTED_CONTINUOUSLY:
        if (rendering_dynamics != RenderingDynamics::STATIC) {
            THROW_OR_ABORT("Instances require static rendering dynamics");
        }
        add_root_instances_always_node(name, std::move(scene_node));
        return;
    }
    THROW_OR_ABORT(
        "Unknown singular rendering strategy: \"" +
        rendering_strategies_to_string(rendering_strategy) + '"');
}

void Scene::add_root_imposter_node(const DanglingRef<SceneNode>& scene_node)
{
    std::scoped_lock lock{ mutex_ };
    scene_node->set_scene_and_state(*this, SceneNodeState::DYNAMIC);
    if (!root_imposter_nodes_.insert(scene_node.ptr()).second)
    {
        THROW_OR_ABORT("Root imposter node already exists");
    }
}

void Scene::move_root_node_to_bvh(const std::string& name) {
    if (static_root_nodes_.contains(name)) {
        static_root_nodes_.move_node_to_bvh(name);
    } else if (root_aggregate_once_nodes_.contains(name)) {
        root_aggregate_once_nodes_.move_node_to_bvh(name);
    } else if (root_aggregate_always_nodes_.contains(name)) {
        root_aggregate_always_nodes_.move_node_to_bvh(name);
    } else {
        root_instances_once_nodes_.move_node_to_bvh(name);
    }
}

bool Scene::root_node_scheduled_for_deletion(
    const std::string& name,
    bool must_exist) const
{
    return morn_.root_node_scheduled_for_deletion(name, must_exist);
}

void Scene::schedule_delete_root_node(const std::string& name) {
    root_nodes_.schedule_delete_root_node(name);
}

void Scene::delete_scheduled_root_nodes() const {
    morn_.delete_scheduled_root_nodes();
}

void Scene::try_delete_root_node(const std::string& name) {
    if (nodes_.contains(name)) {
        delete_root_node(name);
    }
}

void Scene::delete_root_imposter_node(const DanglingRef<SceneNode>& scene_node) {
    std::scoped_lock lock{ mutex_ };
    scene_node->shutdown();
    if (root_imposter_nodes_.erase(scene_node.ptr()) != 1) {
        verbose_abort("Could not delete root imposter node");
    }
}

void Scene::delete_root_node(const std::string& name) {
    LOG_FUNCTION("Scene::delete_root_node");
    root_nodes_.delete_root_node(name);
}

void Scene::delete_root_nodes(const Mlib::regex& regex) {
    LOG_FUNCTION("Scene::delete_root_nodes");
    root_nodes_.delete_root_nodes(regex);
}

void Scene::try_delete_node(const std::string& name) {
    delete_node_mutex_.assert_this_thread_is_deleter_thread();
    if (nodes_.contains(name)) {
        delete_node(name);
    }
}

void Scene::delete_node(const std::string& name) {
    delete_node_mutex_.assert_this_thread_is_deleter_thread();
    DanglingPtr<SceneNode> node = get_node_that_may_be_scheduled_for_deletion(name).ptr();
    if (!node->shutting_down()) {
        if (node->has_parent()) {
            DanglingRef<SceneNode> parent = node->parent();
            node = nullptr;
            parent->remove_child(name);
        } else {
            node = nullptr;
            delete_root_node(name);
        }
    }
}

void Scene::delete_nodes(const Mlib::regex& regex) {
    delete_node_mutex_.assert_this_thread_is_deleter_thread();
    std::unique_lock lock{ mutex_ };
    for (auto it = nodes_.begin(); it != nodes_.end(); ) {
        auto n = it++;
        if (Mlib::re::regex_match(n->first, regex)) {
            UnlockGuard ulock{ lock };
            delete_node(n->first);
        }
    }
}

Scene::~Scene() {
    shutdown();
}

void Scene::shutdown() {
    if (shutting_down_) {
        return;
    }
    stop_and_join();
    delete_node_mutex_.clear_deleter_thread();
    delete_node_mutex_.set_deleter_thread();
    shutting_down_ = true;
    morn_.shutdown();
    if (!nodes_.empty()) {
        for (const auto& [name, _] : nodes_) {
            lerr() << name;
        }
        verbose_abort("Registered nodes remain after shutdown");
    }
    if (!root_imposter_nodes_.empty()) {
        verbose_abort("Imposter nodes remain after shutdown");
    }
    size_t nremaining = try_empty_the_trash_can();
    if (nremaining != 0) {
        for (const auto& o : trash_can_child_nodes_) {
            o.print_references();
        }
        for (const auto& o : trash_can_obj_) {
            o->print_references();
        }
        morn_.print_trash_can_references();
        verbose_abort("Dangling references remain after scene shutdown");
    }
}

void Scene::stop_and_join() {
    large_aggregate_bg_worker_.shutdown();
    large_instances_bg_worker_.shutdown();
    small_aggregate_bg_worker_.shutdown();
    small_instances_bg_worker_.shutdown();
}

void Scene::wait_until_done() const {
    std::shared_lock lock{ mutex_ };
    large_aggregate_bg_worker_.wait_until_done();
    large_instances_bg_worker_.wait_until_done();
    small_aggregate_bg_worker_.wait_until_done();
    small_instances_bg_worker_.wait_until_done();
}

bool Scene::contains_node(const std::string& name) const {
    std::shared_lock lock{ mutex_ };
    return nodes_.contains(name);
}

void Scene::add_to_trash_can(DanglingUniquePtr<SceneNode>&& node) {
    delete_node_mutex_.assert_this_thread_is_deleter_thread();
    trash_can_child_nodes_.emplace_back(std::move(node));
}

void Scene::add_to_trash_can(std::unique_ptr<DanglingBaseClass>&& obj) {
    delete_node_mutex_.assert_this_thread_is_deleter_thread();
    trash_can_obj_.push_back(std::move(obj));
}

size_t Scene::try_empty_the_trash_can() {
    delete_node_mutex_.assert_this_thread_is_deleter_thread();
    for (auto it = trash_can_obj_.begin(); it != trash_can_obj_.end();) {
        auto c = it++;
        if ((*c)->nreferences() == 0) {
            trash_can_obj_.erase(c);
        }
    }
    for (auto it = trash_can_child_nodes_.begin(); it != trash_can_child_nodes_.end();) {
        auto c = it++;
        if (c->nreferences() == 0) {
            trash_can_child_nodes_.erase(c);
        }
    }
    return trash_can_obj_.size() + trash_can_child_nodes_.size() + morn_.try_empty_the_trash_can();
}

void Scene::register_node(
    const std::string& name,
    const DanglingRef<SceneNode>& scene_node)
{
    std::scoped_lock lock{ mutex_ };
    if (name.empty()) {
        THROW_OR_ABORT("register_node received empty name");
    }
    if (!nodes_.insert({ name, scene_node.ptr() }).second) {
        THROW_OR_ABORT("Scene node with name \"" + name + "\" already exists");
    }
}

void Scene::unregister_node(const std::string& name) {
    std::scoped_lock lock{ mutex_ };
    delete_node_mutex_.assert_this_thread_is_deleter_thread();
    if (nodes_.erase(name) != 1) {
        verbose_abort("Could not find node with name (0) \"" + name + '"');
    }
}

void Scene::unregister_nodes(const Mlib::regex& regex) {
    std::scoped_lock lock{ mutex_ };
    delete_node_mutex_.assert_this_thread_is_deleter_thread();
    for (auto it = nodes_.begin(); it != nodes_.end(); ) {
        auto n = *it++;
        if (Mlib::re::regex_match(n.first, regex)) {
            if (nodes_.erase(n.first) != 1) {
                verbose_abort("Could not find node with name (1) \"" + n.first + '"');
            }
        }
    }
}

DanglingRef<SceneNode> Scene::get_node(const std::string& name, SOURCE_LOCATION loc) const {
    std::shared_lock lock{ mutex_ };
    if (delete_node_mutex_.this_thread_is_deleter_thread() &&
        morn_.root_node_scheduled_for_deletion(name, false))
    {
        THROW_OR_ABORT("Node \"" + name + "\" is scheduled for deletion");
    }
    auto res = get_node_that_may_be_scheduled_for_deletion(name).set_loc(loc);
    // Only for debugging purposes, as it
    // overwrites the debug-message with each call.
    // res->set_debug_message(name);
    return res;
}

DanglingPtr<SceneNode> Scene::try_get_node(const std::string& name, SOURCE_LOCATION loc) const {
    std::shared_lock lock{ mutex_ };
    if (!contains_node(name)) {
        return nullptr;
    }
    return get_node(name, loc).ptr();
}

std::list<std::pair<std::string, DanglingRef<SceneNode>>> Scene::get_nodes(const Mlib::regex& regex) const {
    std::shared_lock lock{ mutex_ };
    std::list<std::pair<std::string, DanglingRef<SceneNode>>> result;
    for (const auto& [name, node] : nodes_) {
        if (Mlib::re::regex_match(name, regex)) {
            if (morn_.root_node_scheduled_for_deletion(name, false)) {
                THROW_OR_ABORT("Node \"" + name + "\" is scheduled for deletion");
            }
            result.emplace_back(name, *node);
        }
    }
    return result;
}

DanglingRef<SceneNode> Scene::get_node_that_may_be_scheduled_for_deletion(const std::string& name) const {
    std::shared_lock lock{ mutex_ };
    auto it = nodes_.find(name);
    if (it == nodes_.end()) {
        THROW_OR_ABORT("Could not find node with name (2) \"" + name + '"');
    }
    return *it->second;
}

bool Scene::visit_all(const std::function<bool(
    const TransformationMatrix<float, ScenePos, 3>& m,
    const std::unordered_map<VariableAndHash<std::string>, std::shared_ptr<RenderableWithStyle>>& renderables)>& func) const
{
    std::shared_lock lock{ mutex_ };
    return
        root_nodes_.visit_all([&func](const auto& node) {
            return node->visit_all(TransformationMatrix<float, ScenePos, 3>::identity(), func);
            }) &&
        static_root_nodes_.visit_all([&func](const auto& node){
            return node->visit_all(TransformationMatrix<float, ScenePos, 3>::identity(), func);
            }) &&
        root_aggregate_once_nodes_.visit_all([&func](const auto& node){
            return node->visit_all(TransformationMatrix<float, ScenePos, 3>::identity(), func);
            }) &&
        root_aggregate_always_nodes_.visit_all([&func](const auto& node){
            return node->visit_all(TransformationMatrix<float, ScenePos, 3>::identity(), func);
            }) &&
        root_instances_once_nodes_.visit_all([&func](const auto& node){
            return node->visit_all(TransformationMatrix<float, ScenePos, 3>::identity(), func);
            }) &&
        root_instances_always_nodes_.visit_all([&func](const auto& node){
        return node->visit_all(TransformationMatrix<float, ScenePos, 3>::identity(), func);
            });
}

void Scene::render(
    const FixedArray<ScenePos, 4, 4>& vp,
    const TransformationMatrix<float, ScenePos, 3>& iv,
    const DanglingPtr<const SceneNode>& camera_node,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    const ExternalRenderPass& external_render_pass,
    const std::function<std::function<void()>(std::function<void()>)>& run_in_background) const
{
    // AperiodicLagFinder lag_finder{ "Render: ", std::chrono::milliseconds{5} };
    LOG_FUNCTION("Scene::render");
    std::list<std::pair<TransformationMatrix<float, ScenePos, 3>, std::shared_ptr<Light>>> lights;
    std::list<std::pair<TransformationMatrix<float, ScenePos, 3>, std::shared_ptr<Skidmark>>> skidmarks;
    std::list<Blended> blended;
    std::list<const ColorStyle*> color_styles;
    {
        for (const auto& s : color_styles_.shared()) {
            color_styles.push_back(s.get());
        }
    }
    if (external_render_pass.pass == ExternalRenderPassType::LIGHTMAP_BLACK_NODE) {
        DanglingRef<SceneNode> node = [this, &external_render_pass](){
            std::shared_lock lock{ mutex_ };
            auto res = root_nodes_.try_get(external_render_pass.black_node_name, DP_LOC);
            if (!res.has_value()) {
                THROW_OR_ABORT("Could not find black node with name \"" + external_render_pass.black_node_name + '"');
            }
            return *res;
        }();
        node->render(vp, TransformationMatrix<float, ScenePos, 3>::identity(), iv, camera_node, nullptr, lights, skidmarks, blended, render_config, scene_graph_config, external_render_pass, nullptr, color_styles);
    } else if (external_render_pass.pass == ExternalRenderPassType::LIGHTMAP_BLACK_MOVABLES) {
        NodeDanglingPtrs nodes{ CHUNK_SIZE };
        {
            std::shared_lock lock{ mutex_ };
            root_nodes_.visit(iv.t, [&nodes](const auto& node) { nodes.emplace_back(node.ptr()); return true; });
        }
        for (const auto& node : nodes) {
            node->render(vp, TransformationMatrix<float, ScenePos, 3>::identity(), iv, camera_node, {}, lights, skidmarks, blended, render_config, scene_graph_config, external_render_pass, nullptr, color_styles);
        }
    } else {
        if (!external_render_pass.black_node_name.empty()) {
            THROW_OR_ABORT("Expected empty black node");
        }
        // |         |Lights|Blended|Large|Small|Move|
        // |---------|------|-------|-----|-----|----|
        // |Dynamic  |x     |x      |     |     |x   |
        // |Static   |x     |x      |x    |x    |    |
        // |Aggregate|      |       |x    |x    |    |
        LOG_INFO("Scene::render lights");
        NodeDanglingPtrs local_root_nodes{ CHUNK_SIZE };
        NodeRawPtrs local_static_root_nodes{ CHUNK_SIZE };
        {
            std::shared_lock lock{ mutex_ };
            root_nodes_.visit(iv.t, [&local_root_nodes](const auto& node) { local_root_nodes.emplace_back(node.ptr()); return true; });
            if (any(external_render_pass.pass & ExternalRenderPassType::IS_STATIC_MASK)) {
                static_root_nodes_.visit(iv.t, [&local_static_root_nodes](const auto& node) { local_static_root_nodes.emplace_back(&node.obj()); return true; });
            }
        }
        for (const auto& node : local_root_nodes) {
            node->append_lights_to_queue(TransformationMatrix<float, ScenePos, 3>::identity(), lights);
            node->append_skidmarks_to_queue(TransformationMatrix<float, ScenePos, 3>::identity(), skidmarks);
        }
        for (const auto& node : local_static_root_nodes) {
            node->append_lights_to_queue(TransformationMatrix<float, ScenePos, 3>::identity(), lights);
        }
        if (any(external_render_pass.pass & ExternalRenderPassType::IMPOSTER_OR_ZOOM_NODE)) {
            if (external_render_pass.singular_node == nullptr) {
                THROW_OR_ABORT("Imposter or singular node pass without a singular node");
            }
            auto parent_m = external_render_pass.singular_node->has_parent()
                ? external_render_pass.singular_node->parent()->absolute_model_matrix()
                : TransformationMatrix<float, ScenePos, 3>::identity();
            auto parent_mvp = dot2d(vp, parent_m.affine());
            external_render_pass.singular_node->render(parent_mvp, parent_m, iv, camera_node, {}, lights, skidmarks, blended, render_config, scene_graph_config, external_render_pass, nullptr, color_styles);
        } else {
            if (dynamic_lights_ != nullptr) {
                dynamic_lights_->set_time(external_render_pass.time);
            }
            LOG_INFO("Scene::render non-blended");
            for (const auto& node : local_root_nodes) {
                node->render(vp, TransformationMatrix<float, ScenePos, 3>::identity(), iv, camera_node, dynamic_lights_, lights, skidmarks, blended, render_config, scene_graph_config, external_render_pass, nullptr, color_styles);
            }
            for (const auto& node : local_static_root_nodes) {
                node->render(vp, TransformationMatrix<float, ScenePos, 3>::identity(), iv, nullptr, dynamic_lights_, lights, skidmarks, blended, render_config, scene_graph_config, external_render_pass, nullptr, color_styles);
            }
            {
                NodeDanglingPtrs cached_imposter_nodes{ CHUNK_SIZE };
                {
                    std::shared_lock lock{ mutex_ };
                    for (const auto& node : root_imposter_nodes_) {
                        cached_imposter_nodes.emplace_back(node);
                    }
                }
                for (const auto& node : cached_imposter_nodes) {
                    node->render(vp, TransformationMatrix<float, ScenePos, 3>::identity(), iv, camera_node, dynamic_lights_, lights, skidmarks, blended, render_config, scene_graph_config, external_render_pass, nullptr, color_styles);
                }
            }
            {
                bool is_foreground_task = any(external_render_pass.pass & ExternalRenderPassType::IS_GLOBAL_MASK);
                bool is_background_task = (external_render_pass.pass == ExternalRenderPassType::STANDARD);
                if (is_foreground_task && is_background_task) {
                    THROW_OR_ABORT("Scene::render has both foreground and background task");
                }

                std::shared_ptr<IAggregateRenderer> large_aggregate_renderer = IAggregateRenderer::large_aggregate_renderer();
                if (large_aggregate_renderer != nullptr) {
                    LOG_INFO("Scene::render large_aggregate_renderer");
                    auto large_aggregate_renderer_update_func = [&](TaskLocation task_location){
                        // copy "vp" and "scene_graph_config"
                        return run_in_background([this, iv, scene_graph_config, external_render_pass, large_aggregate_renderer, task_location](){
                            NodeRawPtrs nodes{ CHUNK_SIZE };
                            {
                                std::shared_lock lock{ mutex_ };
                                root_aggregate_once_nodes_.visit(iv.t, [&nodes](const auto& node) { nodes.emplace_back(&node.obj()); return true; });
                            }
                            std::list<std::shared_ptr<ColoredVertexArray<float>>> aggregate_queue;
                            for (const auto& node : nodes) {
                                node->append_large_aggregates_to_queue(TransformationMatrix<float, ScenePos, 3>::identity(), iv.t, aggregate_queue, scene_graph_config);
                            }
                            large_aggregate_renderer->update_aggregates(iv.t, aggregate_queue, external_render_pass, task_location);
                        });
                    };
                    if (is_foreground_task || (is_background_task && !large_aggregate_renderer->is_initialized())) {
                        large_aggregate_bg_worker_.wait_until_done();
                        large_aggregate_renderer_update_func(TaskLocation::FOREGROUND)();
                    } else if (is_background_task && large_aggregate_bg_worker_.done()) {
                        auto dist = sum(squared(large_aggregate_renderer->offset() - iv.t));
                        if (dist > squared(scene_graph_config.large_max_offset_deviation)) {
                            large_aggregate_bg_worker_.run(large_aggregate_renderer_update_func(TaskLocation::BACKGROUND));
                        }
                    }
                    // AperiodicLagFinder lag_finder{ "Large aggregates: ", std::chrono::milliseconds{5} };
                    large_aggregate_renderer->render_aggregates(vp, iv, lights, skidmarks, scene_graph_config, render_config, external_render_pass, color_styles);
                }

                std::shared_ptr<IInstancesRenderer> large_instances_renderer = IInstancesRenderer::large_instances_renderer();
                if (large_instances_renderer != nullptr) {
                    LOG_INFO("Scene::render large_instances_renderer");
                    auto large_instances_renderer_update_func = [&](TaskLocation task_location){
                        // copy "vp" and "scene_graph_config"
                        return run_in_background([this, vp, iv, scene_graph_config, external_render_pass,
                                                  large_instances_renderer, task_location]()
                        {
                            NodeRawPtrs nodes{ CHUNK_SIZE };
                            {
                                std::shared_lock lock{ mutex_ };
                                root_instances_once_nodes_.visit(iv.t, [&nodes](const auto& node) { nodes.emplace_back(&node.obj()); return true; });
                            }
                            LargeInstancesQueue instances_queue{external_render_pass.pass};
                            for (const auto& node : nodes) {
                                node->append_large_instances_to_queue(vp, TransformationMatrix<float, ScenePos, 3>::identity(), iv.t, PositionAndYAngleAndBillboardId{fixed_zeros<CompressedScenePos, 3>(), BILLBOARD_ID_NONE, 0.f}, instances_queue, scene_graph_config);
                            }
                            large_instances_renderer->update_instances(iv.t, instances_queue.queue(), task_location);
                        });
                    };
                    if (is_foreground_task || (is_background_task && !large_instances_renderer->is_initialized())) {
                        large_instances_bg_worker_.wait_until_done();
                        large_instances_renderer_update_func(TaskLocation::FOREGROUND)();
                    } else if (is_background_task && large_instances_bg_worker_.done()) {
                        auto dist = sum(squared(large_instances_renderer->offset() - iv.t));
                        if (dist > squared(scene_graph_config.large_max_offset_deviation)) {
                            large_instances_bg_worker_.run(large_instances_renderer_update_func(TaskLocation::BACKGROUND));
                        }
                    }
                    // AperiodicLagFinder lag_finder{ "large instances: ", std::chrono::milliseconds{5} };
                    large_instances_renderer->render_instances(vp, iv, lights, skidmarks, scene_graph_config, render_config, external_render_pass);
                }

                std::shared_ptr<IAggregateRenderer> small_sorted_aggregate_renderer = IAggregateRenderer::small_sorted_aggregate_renderer();
                if (small_sorted_aggregate_renderer != nullptr) {
                    // Contains continuous alpha and must therefore be rendered late.
                    LOG_INFO("Scene::render small_sorted_aggregate_renderer");
                    auto small_sorted_aggregate_renderer_update_func = [&](TaskLocation task_location){
                        // copy "vp" and "scene_graph_config"
                        return run_in_background([this, vp, iv, scene_graph_config, external_render_pass, small_sorted_aggregate_renderer, task_location](){
                            NodeRawPtrs nodes{ CHUNK_SIZE };
                            {
                                std::shared_lock lock{ mutex_ };
                                root_aggregate_always_nodes_.visit(iv.t, [&nodes](const auto& node) { nodes.emplace_back(&node.obj()); return true; });
                            }
                            std::list<std::pair<float, std::shared_ptr<ColoredVertexArray<float>>>> aggregate_queue;
                            for (const auto& node : nodes) {
                                node->append_sorted_aggregates_to_queue(vp, TransformationMatrix<float, ScenePos, 3>::identity(), iv.t, aggregate_queue, scene_graph_config, external_render_pass);
                            }
                            aggregate_queue.sort([](auto& a, auto& b){ return a.first < b.first; });
                            std::list<std::shared_ptr<ColoredVertexArray<float>>> sorted_aggregate_queue;
                            for (auto& e : aggregate_queue) {
                                sorted_aggregate_queue.push_back(std::move(e.second));
                            }
                            small_sorted_aggregate_renderer->update_aggregates(iv.t, sorted_aggregate_queue, external_render_pass, task_location);
                        });
                    };
                    if (is_foreground_task || (is_background_task && !small_sorted_aggregate_renderer->is_initialized())) {
                        small_aggregate_bg_worker_.wait_until_done();
                        small_sorted_aggregate_renderer_update_func(TaskLocation::FOREGROUND)();
                    } else if (is_background_task && small_aggregate_bg_worker_.done()) {
                        WorkerStatus status = small_aggregate_bg_worker_.tick(scene_graph_config.small_aggregate_update_interval);
                        if (status == WorkerStatus::IDLE) {
                            small_aggregate_bg_worker_.run(small_sorted_aggregate_renderer_update_func(TaskLocation::BACKGROUND));
                        }
                    }
                    // AperiodicLagFinder lag_finder{ "Small sorted aggregates: ", std::chrono::milliseconds{5} };
                    small_sorted_aggregate_renderer->render_aggregates(vp, iv, lights, skidmarks, scene_graph_config, render_config, external_render_pass, color_styles);
                }

                // Contains continuous alpha and must therefore be rendered late.
                LOG_INFO("Scene::render instances_renderer");
                std::shared_ptr<IInstancesRenderers> small_sorted_instances_renderers = IInstancesRenderer::small_sorted_instances_renderers();
                if (small_sorted_instances_renderers != nullptr) {
                    if ((external_render_pass.pass == ExternalRenderPassType::STANDARD) ||
                        any(external_render_pass.pass & ExternalRenderPassType::IS_GLOBAL_MASK))
                    {
                        auto small_instances_renderer_update_func = [&](TaskLocation task_location){
                            std::set<ExternalRenderPassType> black_render_passes;
                            if (external_render_pass.pass == ExternalRenderPassType::STANDARD) {
                                for (const auto &[_, l] : lights) {
                                    if (any(l->shadow_render_pass & ExternalRenderPassType::LIGHTMAP_IS_LOCAL_MASK)) {
                                        black_render_passes.insert(l->shadow_render_pass);
                                    }
                                }
                            }
                            // copy "vp" and "scene_graph_config"
                            return run_in_background([this, vp, iv, scene_graph_config, external_render_pass,
                                                      small_sorted_instances_renderers, task_location,
                                                      black_render_passes]()
                            {
                                NodeRawPtrs nodes{ CHUNK_SIZE };
                                {
                                    std::shared_lock lock{ mutex_ };
                                    root_instances_always_nodes_.visit(iv.t, [&nodes](const auto& node) { nodes.emplace_back(&node.obj()); return true; });
                                }
                                // auto start_time = std::chrono::steady_clock::now();
                                SmallInstancesQueues instances_queues{
                                    external_render_pass.pass,
                                    black_render_passes};
                                for (const auto& node : nodes) {
                                    node->append_small_instances_to_queue(vp, TransformationMatrix<float, ScenePos, 3>::identity(), iv, iv.t, PositionAndYAngleAndBillboardId{fixed_zeros<CompressedScenePos, 3>(), BILLBOARD_ID_NONE, 0.f}, instances_queues, scene_graph_config);
                                }
                                auto sorted_instances = instances_queues.sorted_instances();
                                small_sorted_instances_renderers->get_instances_renderer(external_render_pass.pass)->update_instances(
                                    iv.t,
                                    sorted_instances.at(external_render_pass.pass),
                                    task_location);
                                for (auto rp : black_render_passes) {
                                    small_sorted_instances_renderers->get_instances_renderer(rp)->update_instances(
                                        iv.t,
                                        sorted_instances.at(rp),
                                        task_location);
                                }
                                // lerr() << this << " " << external_render_pass.pass << ", elapsed time: " << std::chrono::duration<float>(std::chrono::steady_clock::now() - start_time).count() << " s";
                            });
                        };
                        if (is_foreground_task || (is_background_task && !small_sorted_instances_renderers->get_instances_renderer(external_render_pass.pass)->is_initialized())) {
                            small_instances_bg_worker_.wait_until_done();
                            small_instances_renderer_update_func(TaskLocation::FOREGROUND)();
                        } else if (is_background_task && small_instances_bg_worker_.done()) {
                            WorkerStatus status = small_instances_bg_worker_.tick(scene_graph_config.small_aggregate_update_interval);
                            if (status == WorkerStatus::IDLE) {
                                small_instances_bg_worker_.run(
                                    small_instances_renderer_update_func(TaskLocation::BACKGROUND));
                            }
                        }
                    }
                    // AperiodicLagFinder lag_finder{ "Small sorted instances: ", std::chrono::milliseconds{5} };
                    small_sorted_instances_renderers->get_instances_renderer(external_render_pass.pass)->render_instances(
                        vp, iv, lights, skidmarks, scene_graph_config, render_config, external_render_pass);
                }
            }
            if (external_render_pass.pass == ExternalRenderPassType::STANDARD) {
                if (particle_renderer_ != nullptr) {
                    // AperiodicLagFinder lag_finder{ "particles: ", std::chrono::milliseconds{5} };
                    particle_renderer_->render(
                        ParticleSubstrate::AIR,
                        vp,
                        iv,
                        lights,
                        skidmarks,
                        scene_graph_config,
                        render_config,
                        external_render_pass);
                }
                if (trail_renderer_ != nullptr) {
                    trail_renderer_->render(
                        vp,
                        iv,
                        lights,
                        skidmarks,
                        scene_graph_config,
                        render_config,
                        external_render_pass);
                }
            }
            {
                // AperiodicLagFinder lag_finder{ "blended: ", std::chrono::milliseconds{5} };
                // Contains continuous alpha and must therefore be rendered late.
                LOG_INFO("Scene::render blended");
                blended.sort([](Blended& a, Blended& b){ return a.sorting_key() > b.sorting_key(); });
                for (const auto& b : blended) {
                    DynamicStyle dynamic_style{ dynamic_lights_ != nullptr
                        ? dynamic_lights_->get_color(b.m.t)
                        : fixed_zeros<float, 3>() };
                    b.renderable().render(
                        b.mvp,
                        b.m,
                        iv,
                        &dynamic_style,
                        lights,
                        skidmarks,
                        scene_graph_config,
                        render_config,
                        { external_render_pass, InternalRenderPass::BLENDED },
                        b.animation_state.get(),
                        b.color_style);
                }
            }
        }
    }
}

void Scene::move(float dt, std::chrono::steady_clock::time_point time) {
    LOG_FUNCTION("Scene::move");
    delete_node_mutex_.assert_this_thread_is_deleter_thread();
    {
        std::unique_lock lock{mutex_};
        if (!morn_.no_root_nodes_scheduled_for_deletion()) {
            THROW_OR_ABORT("Moving with root nodes scheduled for deletion");
        }
        {
            auto &drn = root_nodes_.default_nodes();
            for (auto it = drn.begin(); it != drn.end();) {
                it->second->move(
                    TransformationMatrix<float, ScenePos, 3>::identity(),
                    dt,
                    time,
                    scene_node_resources_,
                    nullptr);  // animation_state
                if (it->second->to_be_deleted()) {
                    UnlockGuard ug{lock};
                    delete_root_node((it++)->first);
                } else {
                    ++it;
                }
            }
        }
        if (dynamic_lights_ != nullptr) {
            dynamic_lights_->append_time(time);
        }
        // times_.append(time);
    }
    try_empty_the_trash_can();
}

void Scene::append_static_filtered_to_queue(
    std::list<std::pair<TransformationMatrix<float, ScenePos, 3>, std::shared_ptr<ColoredVertexArray<float>>>>& float_queue,
    std::list<std::pair<TransformationMatrix<float, ScenePos, 3>, std::shared_ptr<ColoredVertexArray<CompressedScenePos>>>>& double_queue,
    const ColoredVertexArrayFilter& filter) const
{
    LOG_FUNCTION("Scene::append_static_filtered_to_queue");
    std::shared_lock lock{ mutex_ };
    static_root_nodes_.visit_all([&](const auto& node) {
        node->append_static_filtered_to_queue(
            TransformationMatrix<float, ScenePos, 3>::identity(),
            float_queue,
            double_queue,
            filter);
        return true;
        });
}

size_t Scene::get_uuid() {
    std::scoped_lock lock{ uuid_mutex_ };
    return uuid_++;
}

std::string Scene::get_temporary_instance_suffix() {
    return "___" + std::to_string(get_uuid());
}

void Scene::print(std::ostream& ostr) const {
    std::shared_lock lock{ mutex_ };
    ostr << "Scene\n";
    ostr << morn_;
}

bool Scene::shutting_down() const {
    return shutting_down_;
}

void Scene::add_color_style(std::unique_ptr<ColorStyle>&& color_style) {
    color_style->compute_hash();
    color_styles_.push_back(std::move(color_style));
}

DeleteNodeMutex& Scene::delete_node_mutex() const {
    return delete_node_mutex_;
}

IParticleCreator& Scene::particle_instantiator(const VariableAndHash<std::string>& resource_name) const {
    std::shared_lock lock{ mutex_ };
    if (particle_renderer_ == nullptr) {
        THROW_OR_ABORT("Particle renderer not set");
    }
    return particle_renderer_->get_instantiator(resource_name);
}

void Scene::wait_for_cleanup() const {
    while (ncleanups_required_ > 0);
}

void Scene::notify_cleanup_required() {
    ++ncleanups_required_;
}

void Scene::notify_cleanup_done() {
    --ncleanups_required_;
}

std::ostream& Mlib::operator << (std::ostream& ostr, const Scene& scene) {
    scene.print(ostr);
    return ostr;
}
