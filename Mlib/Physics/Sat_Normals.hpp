#pragma once
#include <Mlib/Array/Array_Forward.hpp>
#include <Mlib/Physics/Typed_Mesh.hpp>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace Mlib {

class RigidBody;
template <class TData, size_t n>
class PlaneNd;
class TransformedMesh;

class SatTracker {
public:
    void get_collision_plane(
        const std::shared_ptr<RigidBody>& o0,
        const std::shared_ptr<RigidBody>& o1,
        const std::shared_ptr<TransformedMesh>& mesh0,
        const std::shared_ptr<TransformedMesh>& mesh1,
        float& min_overlap,
        PlaneNd<float, 3>& plane) const;
    void clear();
private:
    mutable std::map<
        std::shared_ptr<RigidBody>,
        std::map<
            std::shared_ptr<RigidBody>,
            std::map<
                std::shared_ptr<TransformedMesh>,
                std::map<std::shared_ptr<TransformedMesh>,
                    std::pair<float, PlaneNd<float, 3>>>>>> collision_planes_;
};

}
