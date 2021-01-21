#include "Aggregate_Array_Renderer.hpp"
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Render/Rendering_Resources.hpp>
#include <map>

using namespace Mlib;

AggregateArrayRenderer::AggregateArrayRenderer()
: rcva_{nullptr},
  rendering_resources_{RenderingResources::rendering_resources()}
{}

void AggregateArrayRenderer::update_aggregates(const std::list<std::shared_ptr<ColoredVertexArray>>& sorted_aggregate_queue) {
    if (rendering_resources_ != RenderingResources::rendering_resources()) {
        std::cerr << rendering_resources_ << std::endl;
        std::cerr << rendering_resources_->name() << " " << RenderingResources::rendering_resources()->name() << std::endl;
        assert_true(false);
    }
    // size_t ntris = 0;
    // for (const auto& a : sorted_aggregate_queue) {
    //     ntris += a->triangles.size();
    // }
    // std::cerr << "Update aggregates: " << ntris << std::endl;

    //std::map<Material, size_t> material_ids;
    //size_t ntriangles = 0;
    //for (const auto& a : sorted_aggregate_queue) {
    //    if (material_ids.find(a.second.material) == material_ids.end()) {
    //        material_ids.insert(std::make_pair(a.second.material, material_ids.size()));
    //    }
    //    ntriangles += a.second.triangles->size();
    //}
    std::map<Material, std::list<FixedArray<ColoredVertex, 3>>> mat_lists;
    for (const auto& a : sorted_aggregate_queue) {
        auto mat = a->material;
        mat.aggregate_mode = AggregateMode::OFF;
        mat.is_small = false;
        auto& l = mat_lists[mat];
        for (const auto& c : a->triangles) {
            l.push_back(c);
        }
    }
    std::list<std::shared_ptr<ColoredVertexArray>> mat_vectors;
    for (auto& l : mat_lists) {
        mat_vectors.push_back(std::make_shared<ColoredVertexArray>(
            "AggregateArrayRenderer",
            l.first,
            std::vector<FixedArray<ColoredVertex, 3>>{l.second.begin(), l.second.end()},
            std::vector<FixedArray<ColoredVertex, 2>>{},
            std::vector<FixedArray<std::vector<BoneWeight>, 3>>{},
            std::vector<FixedArray<std::vector<BoneWeight>, 2>>{}));
    }
    sort_for_rendering(mat_vectors);
    auto rcva = std::make_shared<RenderableColoredVertexArray>(mat_vectors, nullptr);
    auto rcvai = std::make_unique<RenderableColoredVertexArrayInstance>(rcva, SceneNodeResourceFilter{});
    {
        std::lock_guard<std::mutex> lock_guard{mutex_};
        std::swap(rcva_, rcva);
        std::swap(rcvai_, rcvai);
        is_initialized_ = true;
    }
    if (rendering_resources_ != RenderingResources::rendering_resources()) {
        std::cerr << rendering_resources_ << std::endl;
        std::cerr << rendering_resources_->name() << " " << RenderingResources::rendering_resources()->name() << std::endl;
        assert_true(false);
    }
}

void AggregateArrayRenderer::render_aggregates(const FixedArray<float, 4, 4>& vp, const TransformationMatrix<float, 3>& iv, const std::list<std::pair<TransformationMatrix<float, 3>, Light*>>& lights, const SceneGraphConfig& scene_graph_config, const RenderConfig& render_config, const ExternalRenderPass& external_render_pass) const {
    std::lock_guard<std::mutex> lock_guard{mutex_};
    if (is_initialized_) {
        if (rendering_resources_ != RenderingResources::rendering_resources()) {
            std::cerr << rendering_resources_ << std::endl;
            std::cerr << rendering_resources_->name() << " " << RenderingResources::rendering_resources()->name() << std::endl;
            assert_true(false);
        }
        rcvai_->render(
            vp,
            TransformationMatrix<float, 3>::identity(),
            iv,
            lights,
            scene_graph_config,
            render_config,
            {external_render_pass, InternalRenderPass::AGGREGATE},
            nullptr);
    }
}
