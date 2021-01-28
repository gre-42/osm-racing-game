#include "Substitution_Info.hpp"
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Render/CHK.hpp>
#include <Mlib/Render/Instance_Handles/Vertex_Array.hpp>
#include <Mlib/Render/Renderables/Renderable_Colored_Vertex_Array.hpp>
#include <Mlib/Threads/Background_Loop.hpp>

#ifdef __GNUC__
    #pragma GCC push_options
    #pragma GCC optimize ("O3")
#endif

using namespace Mlib;

void SubstitutionInfo::delete_triangle(size_t id, FixedArray<ColoredVertex, 3>* ptr) {
    assert(ntriangles > 0);
    if (triangles_local_ids[id] == ntriangles - 1) {
        triangles_local_ids[id] = SIZE_MAX;
        triangles_global_ids[ntriangles - 1] = SIZE_MAX;
    } else if (ntriangles > 1) {
        size_t from = ntriangles - 1;
        size_t to = triangles_local_ids[id];
        assert(to != SIZE_MAX);
        ptr[to] = cva->triangles[triangles_global_ids[from]];
        triangles_local_ids[id] = SIZE_MAX;
        triangles_local_ids[triangles_global_ids[from]] = to;
        triangles_global_ids[to] = triangles_global_ids[from];
        triangles_global_ids[from] = SIZE_MAX;
    }
    --ntriangles;
}

void SubstitutionInfo::insert_triangle(size_t id, FixedArray<ColoredVertex, 3>* ptr) {
    assert(triangles_global_ids[ntriangles] == SIZE_MAX);
    assert(triangles_local_ids[id] == SIZE_MAX);
    triangles_local_ids[id] = ntriangles;
    triangles_global_ids[ntriangles] = id;
    ptr[ntriangles] = cva->triangles[id];
    ++ntriangles;
}

/**
 * From: https://learnopengl.com/Advanced-OpenGL/Advanced-Data
 */
void SubstitutionInfo::delete_triangles_far_away(
    const FixedArray<float, 3>& position,
    const TransformationMatrix<float, 3>& m,
    float draw_distance_add,
    float draw_distance_slop,
    size_t noperations,
    bool run_in_background,
    bool is_static)
{
    if (cva->triangles.empty()) {
        return;
    }
    if (triangles_local_ids.empty()) {
        triangles_local_ids.resize(cva->triangles.size());
        triangles_global_ids.resize(cva->triangles.size());
        for (size_t i = 0; i < triangles_local_ids.size(); ++i) {
            triangles_local_ids[i] = i;
            triangles_global_ids[i] = i;
        }
        current_triangle_id = 0;
        if (is_static) {
            transformed_triangles.resize(cva->triangles.size());
            for (size_t i = 0; i < cva->triangles.size(); ++i) {
                transformed_triangles[i] = cva->triangles[i].applied<FixedArray<float, 3>>([&m](const ColoredVertex& v){return m * v.position;});
            }
        }
    }
    offset_ = current_triangle_id;
    noperations2_ = std::min(current_triangle_id + noperations, cva->triangles.size()) - current_triangle_id;
    if (triangles_local_ids.size() != cva->triangles.size()) {
        throw std::runtime_error("Array length has changed");
    }
    typedef FixedArray<ColoredVertex, 3> Triangle;
    auto func = [this, draw_distance_add, draw_distance_slop, m, position, run_in_background, is_static](){
        Triangle* ptr = nullptr;
        if (!run_in_background) {
            // Must be inside here because CHK requires an OpenGL context
            CHK(ptr = (Triangle*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
        }
        float add2 = squared(draw_distance_add);
        float remove2 = squared(draw_distance_add + draw_distance_slop);
        for (size_t i = 0; i < noperations2_; ++i) {
            FixedArray<float, 3> center;
            if (is_static) {
                center = mean(transformed_triangles[current_triangle_id]);
            } else {
                auto center_local = mean(cva->triangles[current_triangle_id].applied<FixedArray<float, 3>>([](const ColoredVertex& c){return c.position;}));
                center = m * center_local;
            }
            float dist2 = sum(squared(center - position));
            if (triangles_local_ids[current_triangle_id] != SIZE_MAX) {
                if (dist2 > remove2) {
                    if (run_in_background) {
                        if (triangles_to_delete_.size() == triangles_to_delete_.capacity()) {
                            return;
                        }
                        triangles_to_delete_.push_back(current_triangle_id);
                    } else {
                        delete_triangle(current_triangle_id, ptr);
                    }
                }
            } else {
                if (dist2 < add2) {
                    if (run_in_background) {
                        if (triangles_to_insert_.size() == triangles_to_insert_.capacity()) {
                            return;
                        }
                        triangles_to_insert_.push_back(current_triangle_id);
                    } else {
                        insert_triangle(current_triangle_id, ptr);
                    }
                }
            }
            current_triangle_id = (current_triangle_id + 1) % triangles_local_ids.size();
        }
    };
    if (run_in_background) {
        if (background_loop_ == nullptr) {
            background_loop_ = std::make_unique<BackgroundLoop>();
            triangles_to_delete_.reserve(noperations2_);
            triangles_to_insert_.reserve(noperations2_);
        }
        if (triangles_to_delete_.capacity() < noperations2_) {
            throw std::runtime_error("noperations or triangle list changed");
        }
        if (background_loop_->done()) {
            if (!triangles_to_delete_.empty() || !triangles_to_insert_.empty()) {
                CHK(glBindBuffer(GL_ARRAY_BUFFER, va.vertex_buffer));
                // CHK(Triangle* ptr = (Triangle*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
                CHK(Triangle * ptr = (Triangle*)glMapBufferRange(GL_ARRAY_BUFFER, offset_ * sizeof(Triangle), noperations2_ * sizeof(Triangle), GL_MAP_WRITE_BIT));
                ptr -= offset_;
                for (size_t i : triangles_to_delete_) {
                    delete_triangle(i, ptr);
                }
                triangles_to_delete_.clear();
                for (size_t i : triangles_to_insert_) {
                    insert_triangle(i, ptr);
                }
                triangles_to_insert_.clear();
                CHK(glUnmapBuffer(GL_ARRAY_BUFFER));
            }
            background_loop_->run(func);
        }
    } else {
        if (background_loop_ != nullptr) {
            throw std::runtime_error("Substitution both in fg and bg");
        }
        CHK(glBindBuffer(GL_ARRAY_BUFFER, va.vertex_buffer));
        func();
        CHK(glUnmapBuffer(GL_ARRAY_BUFFER));
    }
}

#ifdef __GNUC__
    #pragma GCC pop_options
#endif
