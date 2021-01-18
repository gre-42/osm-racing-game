#pragma once
#include <Mlib/Array/Array_Forward.hpp>
#include <Mlib/Render/Renderables/Renderable_Colored_Vertex_Array.hpp>
#include <Mlib/Scene_Graph/Scene_Node_Resource.hpp>

namespace Mlib {

class RenderableHeightMap: public SceneNodeResource {
public:
    RenderableHeightMap(
        const Array<float>& rgb_picture,
        const Array<float>& height_picture,
        const TransformationMatrix<float, 2>& normalization_matrix);
    virtual void instantiate_renderable(const std::string& name, SceneNode& scene_node, const SceneNodeResourceFilter& resource_filter) const override;
    virtual std::shared_ptr<AnimatedColoredVertexArrays> get_animated_arrays() const override;
    virtual void generate_triangle_rays(size_t npoints, const FixedArray<float, 3>& lengths, bool delete_triangles = false) override;
private:
    std::shared_ptr<RenderableColoredVertexArray> rva_;

};

}
