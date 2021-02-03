#include "Renderable_Blending_X.hpp"
#include <Mlib/Array/Fixed_Array.hpp>
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Geometry/Mesh/Colored_Vertex_Array.hpp>
#include <Mlib/Images/Coordinates_Fixed.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Math/Pi.hpp>
#include <Mlib/Scene_Graph/Scene.hpp>
#include <Mlib/Scene_Graph/Scene_Node.hpp>

using namespace Mlib;

RenderableBlendingX::RenderableBlendingX(
    const FixedArray<float, 2, 2>& square,
    const std::string& texture)
: square_{square}
{
    for (size_t i = 0; i < 2; ++i) {
        ColoredVertex v00{ // min(x), min(y)
                {square(0, 0) / 2, square(0, 1), 0.f},
                fixed_ones<float, 3>(),
                {i / 2.f, 0.f},
                {0.f, 0.f, 1.f}};
        ColoredVertex v01{ // min(x), max(y)
                {square(0, 0) / 2, square(1, 1), 0.f},
                fixed_ones<float, 3>(),
                {i / 2.f, 1.f},
                {0.f, 0.f, 1.f}};
        ColoredVertex v10{ // max(x), min(y)
                {square(1, 0) / 2, square(0, 1), 0.f},
                fixed_ones<float, 3>(),
                {(1 + i) / 2.f, 0.f},
                {0.f, 0.f, 1.f}};
        ColoredVertex v11{ // max(x), max(y)
                {square(1, 0) / 2, square(1, 1), 0.f},
                fixed_ones<float, 3>(),
                {(1 + i) / 2.f, 1.f},
                {0.f, 0.f, 1.f}};

        std::vector<FixedArray<ColoredVertex, 3>> triangles;
        triangles.reserve(2);
        triangles.push_back(FixedArray<ColoredVertex, 3>{v00, v11, v01});
        triangles.push_back(FixedArray<ColoredVertex, 3>{v11, v00, v10});

        rva_(i) = std::make_shared<RenderableColoredVertexArray>(
            std::make_shared<ColoredVertexArray>(
                "RenderableBlendingX",
                Material{
                    .blend_mode = BlendMode::CONTINUOUS,
                    .texture_descriptor = {.color = texture},
                    .occluder_type = OccluderType::OFF,
                    .wrap_mode_s = WrapMode::CLAMP_TO_EDGE,
                    .wrap_mode_t = WrapMode::CLAMP_TO_EDGE,
                    .collide = false,
                    .aggregate_mode = AggregateMode::SORTED_CONTINUOUSLY,
                    .is_small = true,
                    .cull_faces = false,
                    .ambience = {2.f, 2.f, 2.f},
                    .diffusivity = {0.f, 0.f, 0.f},
                    .specularity = {0.f, 0.f, 0.f}}.compute_color_mode(),
                std::move(triangles),
                std::move(std::vector<FixedArray<ColoredVertex, 2>>()),
                std::move(std::vector<FixedArray<std::vector<BoneWeight>, 3>>()),
                std::move(std::vector<FixedArray<std::vector<BoneWeight>, 2>>())),
            nullptr);
    }
}

void RenderableBlendingX::instantiate_renderable(const std::string& name, SceneNode& scene_node, const SceneNodeResourceFilter& resource_filter) const
{
    // std::unique_lock lock_guard0{scene.dynamic_mutex_};
    // std::unique_lock lock_guard1{scene.static_mutex_};
    // std::unique_lock lock_guard2{scene.aggregate_mutex_};
    {
        auto node = new SceneNode;
        node->set_rotation({0.f, 0.f, 0.f });
        node->set_position({(square_(1, 0) - square_(0, 0)) / 4.f, 0.f, 0.f });
        rva_(1)->instantiate_renderable("plane", *node, SceneNodeResourceFilter());
        scene_node.add_aggregate_child(name + "+0", node);
    }
    {
        auto node = new SceneNode;
        node->set_rotation({0.f, 0.f, 0.f });
        node->set_position({-(square_(1, 0) - square_(0, 0)) / 4.f, 0.f, 0.f });
        rva_(0)->instantiate_renderable("plane", *node, SceneNodeResourceFilter());
        scene_node.add_aggregate_child(name + "-0", node);
    }
    {
        auto node = new SceneNode;
        node->set_rotation({0.f, -float{M_PI} / 2, 0.f });
        node->set_position({0.f, 0.f, (square_(1, 1) - square_(0, 1)) / 4.f });
        rva_(1)->instantiate_renderable("plane", *node, SceneNodeResourceFilter());
        scene_node.add_aggregate_child(name + "+1", node);
    }
    {
        auto node = new SceneNode;
        node->set_rotation({0.f, -float{M_PI} / 2, 0.f });
        node->set_position({0.f, 0.f, -(square_(1, 1) - square_(0, 1)) / 4.f });
        rva_(0)->instantiate_renderable("plane", *node, SceneNodeResourceFilter());
        scene_node.add_aggregate_child(name + "-1", node);
    }
}
