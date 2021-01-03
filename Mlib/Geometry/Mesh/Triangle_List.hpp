#pragma once
#include <Mlib/Array/Fixed_Array.hpp>
#include <Mlib/Geometry/Colored_Vertex.hpp>
#include <Mlib/Geometry/Material.hpp>
#include <list>
#include <memory>

namespace Mlib {

class ColoredVertex;
class ColoredVertexArray;

class TriangleList {
public:
    explicit TriangleList(const std::string& name, const Material& material)
    : name_{name},
      material_{material}
    {}
    void draw_triangle_with_normals(
        const FixedArray<float, 3>& p00,
        const FixedArray<float, 3>& p10,
        const FixedArray<float, 3>& p01,
        const FixedArray<float, 3>& n00,
        const FixedArray<float, 3>& n10,
        const FixedArray<float, 3>& n01,
        const FixedArray<float, 3>& c00 = {1.f, 0.f, 0.f},
        const FixedArray<float, 3>& c10 = {0.f, 1.f, 0.f},
        const FixedArray<float, 3>& c01 = {0.f, 0.f, 1.f},
        const FixedArray<float, 2>& u00 = {0.f, 0.f},
        const FixedArray<float, 2>& u10 = {1.f, 0.f},
        const FixedArray<float, 2>& u01 = {0.f, 1.f});
    void draw_triangle_wo_normals(
        const FixedArray<float, 3>& p00,
        const FixedArray<float, 3>& p10,
        const FixedArray<float, 3>& p01,
        const FixedArray<float, 3>& c00 = {1.f, 0.f, 0.f},
        const FixedArray<float, 3>& c10 = {0.f, 1.f, 0.f},
        const FixedArray<float, 3>& c01 = {0.f, 0.f, 1.f},
        const FixedArray<float, 2>& u00 = {0.f, 0.f},
        const FixedArray<float, 2>& u10 = {1.f, 0.f},
        const FixedArray<float, 2>& u01 = {0.f, 1.f});
    void draw_rectangle_with_normals(
        const FixedArray<float, 3>& p00,
        const FixedArray<float, 3>& p10,
        const FixedArray<float, 3>& p11,
        const FixedArray<float, 3>& p01,
        const FixedArray<float, 3>& n00,
        const FixedArray<float, 3>& n10,
        const FixedArray<float, 3>& n11,
        const FixedArray<float, 3>& n01,
        const FixedArray<float, 3>& c00 = {1.f, 0.f, 0.f},
        const FixedArray<float, 3>& c10 = {0.f, 1.f, 0.f},
        const FixedArray<float, 3>& c11 = {0.f, 0.f, 1.f},
        const FixedArray<float, 3>& c01 = {0.f, 1.f, 1.f},
        const FixedArray<float, 2>& u00 = {0.f, 0.f},
        const FixedArray<float, 2>& u10 = {1.f, 0.f},
        const FixedArray<float, 2>& u11 = {1.f, 1.f},
        const FixedArray<float, 2>& u01 = {0.f, 1.f});
    void draw_rectangle_wo_normals(
        const FixedArray<float, 3>& p00,
        const FixedArray<float, 3>& p10,
        const FixedArray<float, 3>& p11,
        const FixedArray<float, 3>& p01,
        const FixedArray<float, 3>& c00 = {1.f, 0.f, 0.f},
        const FixedArray<float, 3>& c10 = {0.f, 1.f, 0.f},
        const FixedArray<float, 3>& c11 = {0.f, 0.f, 1.f},
        const FixedArray<float, 3>& c01 = {0.f, 1.f, 1.f},
        const FixedArray<float, 2>& u00 = {0.f, 0.f},
        const FixedArray<float, 2>& u10 = {1.f, 0.f},
        const FixedArray<float, 2>& u11 = {1.f, 1.f},
        const FixedArray<float, 2>& u01 = {0.f, 1.f});
    static void extrude(const std::list<std::shared_ptr<TriangleList>>& triangle_lists, float height, float scale, float uv_scale_x, float uv_scale_y, TriangleList& dest);
    void delete_backfacing_triangles();
    void calculate_triangle_normals();
    void convert_triangle_to_vertex_normals();
    static void convert_triangle_to_vertex_normals(std::list<std::shared_ptr<TriangleList>>& triangle_lists);
    static void smoothen_edges(
        const std::list<std::shared_ptr<TriangleList>>& edge_triangle_lists,
        const std::list<std::shared_ptr<TriangleList>>& excluded_triangle_lists,
        const std::list<FixedArray<float, 3>*>& smoothed_vertices,
        float smoothness,
        size_t niterations);
    std::list<FixedArray<ColoredVertex, 3>> get_triangles_around(const FixedArray<float, 2>& pt, float radius) const;
    std::shared_ptr<ColoredVertexArray> triangle_array() const;
    std::string name_;
    Material material_;
    std::list<FixedArray<ColoredVertex, 3>> triangles_;
};

}
