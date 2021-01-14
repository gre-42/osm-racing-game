#include "Rigid_Primitives.hpp"
#include <Mlib/Geometry/Fixed_Cross.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Physics/Misc/Rigid_Body.hpp>
#include <Mlib/Physics/Misc/Rigid_Body_Pulses.hpp>

using namespace Mlib;

RigidBodyPulses Mlib::rigid_cuboid_pulses(
    float mass,
    const FixedArray<float, 3>& size,
    const FixedArray<float, 3>& com)
{
    // From: https://de.wikipedia.org/wiki/Steinerscher_Satz
    FixedArray<float, 3, 3> I{
        1.f / 12 * mass * (squared(size(1)) + squared(size(2))), 0, 0,
        0, 1.f / 12 * mass * (squared(size(0)) + squared(size(2))), 0,
        0, 0, 1.f / 12 * mass * (squared(size(0)) + squared(size(1)))};
    
    if (mass != INFINITY) {
        FixedArray<float, 3, 3> a = cross(com);
        I += mass * dot2d(a.T(), a);
    }

    return RigidBodyPulses{
        mass,
        I,                                  // I
        com,                                // com
        fixed_zeros<float, 3>(),            // v
        fixed_zeros<float, 3>(),            // w
        fixed_nans<float, 3>(),             // position
        fixed_zeros<float, 3>(),            // rotation (not NAN to pass rogridues angle assertion)
        count_nonzero(com != 0.f) <= 1      // I_is_diagonal
    };
}

RigidBodyIntegrator Mlib::rigid_cuboid_integrator(
    float mass,
    const FixedArray<float, 3>& size,
    const FixedArray<float, 3>& com)
{
    // From: https://de.wikipedia.org/wiki/Steinerscher_Satz
    FixedArray<float, 3, 3> I{
        1.f / 12 * mass * (squared(size(1)) + squared(size(2))), 0, 0,
        0, 1.f / 12 * mass * (squared(size(0)) + squared(size(2))), 0,
        0, 0, 1.f / 12 * mass * (squared(size(0)) + squared(size(1)))};
    
    if (mass != INFINITY) {
        FixedArray<float, 3, 3> a = cross(com);
        I += mass * dot2d(a.T(), a);
    }

    return RigidBodyIntegrator{
        mass,
        fixed_zeros<float, 3>(),            // L
        I,                                  // I
        com,                                // com
        fixed_zeros<float, 3>(),            // v
        fixed_zeros<float, 3>(),            // w
        fixed_zeros<float, 3>(),            // T
        fixed_nans<float, 3>(),             // position
        fixed_zeros<float, 3>(),            // rotation (not NAN to pass rogridues angle assertion)
        count_nonzero(com != 0.f) <= 1      // I_is_diagonal
    };
}

std::shared_ptr<RigidBody> Mlib::rigid_cuboid(
    RigidBodies& rigid_bodies,
    float mass,
    const FixedArray<float, 3>& size,
    const FixedArray<float, 3>& com,
    const TransformationMatrix<double, 3>* geographic_coordinates)
{
    return std::make_shared<RigidBody>(
        rigid_bodies,
        rigid_cuboid_integrator(mass, size, com),
        geographic_coordinates);
}
