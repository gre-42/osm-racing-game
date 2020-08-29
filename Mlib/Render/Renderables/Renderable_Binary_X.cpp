#include "Renderable_Binary_X.hpp"
#include <Mlib/Array/Fixed_Array.hpp>
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Geometry/Mesh/Colored_Vertex_Array.hpp>
#include <Mlib/Images/Coordinates_Fixed.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Scene_Graph/Scene.hpp>
#include <Mlib/Scene_Graph/Scene_Node.hpp>

using namespace Mlib;

RenderableBinaryX::RenderableBinaryX(
    const FixedArray<float, 2, 2>& square,
    const std::string& texture,
    RenderingResources* rendering_resources,
    bool is_small)
{
    std::vector<FixedArray<ColoredVertex, 3>> triangles;
    triangles.reserve(2);

    ColoredVertex v00{ // min(x), min(y)
            {square(0, 0), square(0, 1), 0},
            fixed_ones<float, 3>(),
            {0, 0},
            {0, 0, 1}};
    ColoredVertex v01{ // min(x), max(y)
            {square(0, 0), square(1, 1), 0},
            fixed_ones<float, 3>(),
            {0, 1},
            {0, 0, 1}};
    ColoredVertex v10{ // max(x), min(y)
            {square(1, 0), square(0, 1), 0},
            fixed_ones<float, 3>(),
            {1,0},
            {0, 0, 1}};
    ColoredVertex v11{ // max(x), max(y)
            {square(1, 0), square(1, 1), 0},
            fixed_ones<float, 3>(),
            {1, 1},
            {0, 0, 1}};

    triangles.push_back(FixedArray<ColoredVertex, 3>{v00, v11, v01});
    triangles.push_back(FixedArray<ColoredVertex, 3>{v11, v00, v10});

    rva_ = std::make_shared<RenderableColoredVertexArray>(
        std::make_shared<ColoredVertexArray>(
                "",
                Material{
                    texture: texture,
                    occluder_type: is_small ? OccluderType::OFF : OccluderType::BLACK,
                    blend_mode: BlendMode::BINARY,
                    clamp_mode_s: ClampMode::EDGE,
                    clamp_mode_t: ClampMode::EDGE,
                    collide: false,
                    aggregate_mode: is_small ? AggregateMode::SORTED_CONTINUOUSLY : AggregateMode::ONCE,
                    is_small: is_small,
                    cull_faces: false,
                    ambience: {2, 2, 2},
                    diffusivity: {0, 0, 0},
                    specularity: {0, 0, 0}},
                std::move(triangles),
                std::move(std::vector<FixedArray<ColoredVertex, 2>>())),
        rendering_resources);
}

void RenderableBinaryX::initialize() {
    rva_->initialize();
}

void RenderableBinaryX::instantiate_renderable(const std::string& name, SceneNode& scene_node, const SceneNodeResourceFilter& resource_filter)
{
    // std::unique_lock lock_guard0{scene.dynamic_mutex_};
    // std::unique_lock lock_guard1{scene.static_mutex_};
    // std::unique_lock lock_guard2{scene.aggregate_mutex_};

    rva_->instantiate_renderable("plane", scene_node, SceneNodeResourceFilter{});

    auto node90 = new SceneNode;
    node90->set_rotation({0, -M_PI / 2, 0});
    rva_->instantiate_renderable("plane", *node90, SceneNodeResourceFilter{});
    scene_node.add_aggregate_child("node90", node90);
}
