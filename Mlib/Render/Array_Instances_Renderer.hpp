#pragma once
#include <Mlib/Render/Renderables/Renderable_Colored_Vertex_Array.hpp>
#include <Mlib/Render/Renderables/Renderable_Colored_Vertex_Array/Renderable_Colored_Vertex_Array_Instance.hpp>
#include <Mlib/Scene_Graph/Instances_Renderer.hpp>
#include <atomic>
#include <mutex>

namespace Mlib {

class RenderingResources;

class SmallInstancesRendererGuard {
public:
    explicit SmallInstancesRendererGuard();
    ~SmallInstancesRendererGuard();
};

class ArrayInstancesRenderer: public InstancesRenderer {
public:
    ArrayInstancesRenderer(const ArrayInstancesRenderer& other) = delete;
    ArrayInstancesRenderer& operator = (const ArrayInstancesRenderer& other) = delete;
    explicit ArrayInstancesRenderer();
    virtual void update_instances(const std::list<TransformedColoredVertexArray>& instances_queue) override;
    virtual void render_instances(const FixedArray<float, 4, 4>& vp, const TransformationMatrix<float, 3>& iv, const std::list<std::pair<TransformationMatrix<float, 3>, Light*>>& lights, const SceneGraphConfig& scene_graph_config, const RenderConfig& render_config, const ExternalRenderPass& external_render_pass) const override;
private:
    std::shared_ptr<RenderableColoredVertexArray> rcva_;
    std::unique_ptr<RenderableColoredVertexArrayInstance> rcvai_;
    mutable std::mutex mutex_;
    bool is_initialized_ = false;
};

}
