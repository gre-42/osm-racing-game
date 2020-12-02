#pragma once
#include <Mlib/Array/Array_Forward.hpp>
#include <Mlib/Render/Renderables/Renderable_Colored_Vertex_Array.hpp>
#include <Mlib/Scene_Graph/Scene_Node_Resource.hpp>

namespace Mlib {

class RenderableBinaryX: public SceneNodeResource {
public:
    RenderableBinaryX(
        const FixedArray<float, 2, 2>& square,
        const std::string& texture,
        RenderingResources* rendering_resources,
        bool is_small,
        OccluderType occluder_type,
        const FixedArray<float, 3>& ambience = {2, 2, 2});
    virtual void instantiate_renderable(const std::string& name, SceneNode& scene_node, const SceneNodeResourceFilter& resource_filter) const override;
    virtual AggregateMode aggregate_mode() const override;
private:
    std::shared_ptr<RenderableColoredVertexArray> rva_;

};

}
