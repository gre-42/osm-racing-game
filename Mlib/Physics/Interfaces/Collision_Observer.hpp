#pragma once
#include <Mlib/Array/Array_Forward.hpp>
#include <Mlib/Physics/Collision_Type.hpp>
#include <list>
#include <memory>

namespace Mlib {

struct RigidBodyPulses;

class CollisionObserver {
public:
    // Called by rigid body's destructor
    virtual ~CollisionObserver() = default;
    virtual void notify_collided(
        const std::list<std::shared_ptr<CollisionObserver>>& collision_observers,
        CollisionType& collision_type,
        bool& abort) {};
    virtual void notify_impact(
        const RigidBodyPulses& rbp,
        const std::list<std::shared_ptr<CollisionObserver>>& collision_observers,
        const FixedArray<float, 3>& normal,
        float lambda_final) {};
};

}
