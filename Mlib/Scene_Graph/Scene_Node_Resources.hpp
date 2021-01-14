#pragma once
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace Mlib {

template <class TData, size_t n>
class TransformationMatrix;
struct BvhEntry;
struct BvhConfig;
class BvhLoader;
class SceneNodeResource;
class SceneNode;
struct SceneNodeResourceFilter;
template <typename TData, size_t... tshape>
class FixedArray;
template <class TData>
class OffsetAndQuaternion;
struct AnimatedColoredVertexArrays;
template <class TData, size_t tndim>
struct PointsAndAdjacency;
enum class AggregateMode;
struct SpawnPoint;
enum class WayPointLocation;

class SceneNodeResources {
public:
    SceneNodeResources();
    ~SceneNodeResources();
    void add_resource(
        const std::string& name,
        const std::shared_ptr<SceneNodeResource>& resource);
    void add_bvh_file(
        const std::string& name,
        const std::string& filename,
        const BvhConfig& bvh_config);
    void instantiate_renderable(
        const std::string& resource_name,
        const std::string& instance_name,
        SceneNode& scene_node,
        const SceneNodeResourceFilter& resource_filter) const;
    void register_geographic_mapping(
        const std::string& resource_name,
        const std::string& instance_name,
        SceneNode& scene_node);
    const TransformationMatrix<double, 3>* get_geographic_mapping(const std::string& name) const;
    std::shared_ptr<AnimatedColoredVertexArrays> get_animated_arrays(const std::string& name) const;
    void generate_triangle_rays(const std::string& name, size_t npoints, const FixedArray<float, 3>& lengths, bool delete_triangles = false) const;
    void generate_ray(const std::string& name, const FixedArray<float, 3>& from, const FixedArray<float, 3>& to) const;
    AggregateMode aggregate_mode(const std::string& name) const;
    std::list<SpawnPoint> spawn_points(const std::string& name) const;
    std::map<WayPointLocation, PointsAndAdjacency<float, 2>> way_points(const std::string& name) const;
    void set_relative_joint_poses(const std::string& name, const std::map<std::string, OffsetAndQuaternion<float>>& poses) const;
    std::map<std::string, OffsetAndQuaternion<float>> get_poses(const std::string& name, float seconds) const;
    void downsample(const std::string& name, size_t factor) const;
    void import_bone_weights(
        const std::string& destination,
        const std::string& source,
        float max_distance) const;
private:
    std::map<std::string, std::shared_ptr<SceneNodeResource>> resources_;
    mutable std::map<std::string, BvhEntry> bvh_loaders_;
    std::map<std::string, TransformationMatrix<double, 3>> geographic_mappings_;
    mutable std::recursive_mutex mutex_;
};

}
