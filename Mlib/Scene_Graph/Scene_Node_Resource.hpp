#pragma once
#include <Mlib/Array/Array_Forward.hpp>
#include <list>
#include <memory>
#include <regex>
#include <vector>

namespace Mlib {

class ColoredVertexArray;
class Scene;
class SceneNode;

struct SceneNodeResourceFilter {
    size_t min_num = 0;
    size_t max_num = SIZE_MAX;
    std::regex regex{""};
    inline bool matches(size_t num, const std::string& name) const {
        return (num >= min_num) && (num <= max_num) && (std::regex_search(name, regex));
    }
};

class SceneNodeResource {
public:
    virtual void initialize() = 0;
    virtual void instantiate_renderable(const std::string& name, SceneNode& scene_node, const SceneNodeResourceFilter& resource_filter)  {
        throw std::runtime_error("instantiate_renderable not implemented");
    }
    virtual std::list<std::shared_ptr<ColoredVertexArray>> get_triangle_meshes()  {
        throw std::runtime_error("get_triangle_meshes not implemented");
    }
    virtual void generate_triangle_rays(size_t npoints, const FixedArray<float, 3>& lengths, bool delete_triangles = false) {
        throw std::runtime_error("generate_triangle_rays not implemented");
    }
    virtual void generate_ray(const FixedArray<float, 3>& from, const FixedArray<float, 3>& to)  {
        throw std::runtime_error("generate_ray not implemented");
    }
};

}
