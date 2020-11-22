#pragma once
#include <Mlib/Array/Fixed_Array.hpp>

namespace Mlib {

class RigidBody;
struct PhysicsEngineConfig;
struct PowerIntent;

FixedArray<float, 3> updated_tire_speed(
    const PowerIntent& P,
    RigidBody& rb,
    const FixedArray<float, 3>& v3,
    const FixedArray<float, 3>& n3,
    float v0,
    const FixedArray<float, 3>& surface_normal,
    const PhysicsEngineConfig& cfg,
    size_t tire_id,
    float& force_min,
    float& force_max);

FixedArray<float, 3> handle_tire_triangle_intersection(
    RigidBody& rb,
    const FixedArray<float, 3>& v3,
    const FixedArray<float, 3>& n3,
    const FixedArray<float, 3>& surface_normal,
    float stiction_force,
    float friction_force,
    const PhysicsEngineConfig& cfg,
    size_t tire_id);

}
