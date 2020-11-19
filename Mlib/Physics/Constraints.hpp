#pragma once
#include <Mlib/Array/Fixed_Array.hpp>
#include <Mlib/Geometry/Plane_Nd.hpp>
#include <Mlib/Physics/Misc/Rigid_Body_Engine.hpp>
#include <list>

namespace Mlib {

class RigidBody;
struct RigidBodyPulses;
struct PhysicsEngineConfig;

struct VelocityConstraint {
    FixedArray<float, 3> normal;
    float b;
    float lambda_total = 0;
    inline float C(const FixedArray<float, 3>& x) const {
        return -dot0d(normal, x);
    }
    inline float v() const {
        return b;
    }
};

struct NormalImpulse {
    FixedArray<float, 3> normal;
    float lambda_total;
};

struct PlaneConstraint {
    NormalImpulse normal_impulse;
    float intercept;
    float b = 0;
    float slop = 0;
    float lambda_total = 0;
    bool always_active = true;
    float beta = 0.5;
    float beta2 = 0.2;
    inline float C(const FixedArray<float, 3>& x) const {
        return -(dot0d(normal_impulse.normal, x) + intercept);
    }
    inline float overlap(const FixedArray<float, 3>& x) const {
        // std::cerr << plane.normal_ << " | " << x << " | " << plane.intercept_ << std::endl;
        return -(dot0d(normal_impulse.normal, x) + intercept);
    }
    inline float active(const FixedArray<float, 3>& x) const {
        return always_active || (overlap(x) > 0);
    }
    inline float bias(const FixedArray<float, 3>& x) const {
        return std::max(0.f, overlap(x) - slop);
    }
    inline float v(const FixedArray<float, 3>& p, float dt) const {
        return b + 1.f / dt * (beta * C(p) - beta2 * bias(p));
    }
};

template <class TConstraint>
struct BoundedConstraint1D {
    TConstraint constraint;
    float lambda_min = -INFINITY;
    float lambda_max = INFINITY;
    inline float clamped_lambda(float lambda) {
        lambda = std::clamp(constraint.normal_impulse.lambda_total + lambda, lambda_min, lambda_max) - constraint.normal_impulse.lambda_total;
        constraint.normal_impulse.lambda_total += lambda;
        return lambda;
    }
};

struct ShockAbsorberConstraint {
    NormalImpulse normal_impulse;
    float distance;
    float Ks;  // K_spring
    float Ka;  // K_absorber
};

typedef BoundedConstraint1D<PlaneConstraint> BoundedPlaneConstraint;
typedef BoundedConstraint1D<ShockAbsorberConstraint> BoundedShockAbsorberConstraint;

class ContactInfo {
public:
    virtual void solve(float dt, float relaxation) = 0;
    virtual void finalize() {}
    virtual ~ContactInfo() = default;
};

class ContactInfo1: public ContactInfo {
public:
    ContactInfo1(
        RigidBodyPulses& rbp,
        const BoundedPlaneConstraint& pc,
        const FixedArray<float, 3>& p);
    void solve(float dt, float relaxation) override;
    const NormalImpulse& normal_impulse() const {
        return pc_.constraint.normal_impulse;
    }
private:
    RigidBodyPulses& rbp_;
    BoundedPlaneConstraint pc_;
    FixedArray<float, 3> p_;
};

class ContactInfo2: public ContactInfo {
public:
    ContactInfo2(
        RigidBodyPulses& rbp0,
        RigidBodyPulses& rbp1,
        const BoundedPlaneConstraint& pc,
        const FixedArray<float, 3>& p);
    void solve(float dt, float relaxation) override;
    const NormalImpulse& normal_impulse() const {
        return pc_.constraint.normal_impulse;
    }
private:
    RigidBodyPulses& rbp0_;
    RigidBodyPulses& rbp1_;
    BoundedPlaneConstraint pc_;
    FixedArray<float, 3> p_;
};

class ShockAbsorberContactInfo1: public ContactInfo {
public:
    ShockAbsorberContactInfo1(
        RigidBodyPulses& rbp,
        const BoundedShockAbsorberConstraint& sc,
        const FixedArray<float, 3>& p);
    void solve(float dt, float relaxation) override;
    const NormalImpulse& normal_impulse() const {
        return sc_.constraint.normal_impulse;
    }
private:
    RigidBodyPulses& rbp_;
    BoundedShockAbsorberConstraint sc_;
    FixedArray<float, 3> p_;
};

class FrictionContactInfo1: public ContactInfo {
public:
    FrictionContactInfo1(
        RigidBodyPulses& rbp,
        const NormalImpulse& normal_impulse,
        const FixedArray<float, 3>& p,
        float stiction_coefficient,
        float friction_coefficient,
        const FixedArray<float, 3>& b);
    void solve(float dt, float relaxation) override;
    float max_impulse() const;
    void set_b(const FixedArray<float, 3>& b);
private:
    FixedArray<float, 3> lambda_total_;
    FixedArray<float, 3> b_;
    RigidBodyPulses& rbp_;
    const NormalImpulse& normal_impulse_;
    FixedArray<float, 3> p_;
    float stiction_coefficient_;
    float friction_coefficient_;
};

class FrictionContactInfo2: public ContactInfo {
public:
    FrictionContactInfo2(
        RigidBodyPulses& rbp0,
        RigidBodyPulses& rbp1,
        const NormalImpulse& normal_impulse,
        const FixedArray<float, 3>& p,
        float stiction_coefficient,
        float friction_coefficient,
        const FixedArray<float, 3>& b);
    void solve(float dt, float relaxation) override;
    float max_impulse() const;
    void set_b(const FixedArray<float, 3>& b);
private:
    FixedArray<float, 3> lambda_total_;
    FixedArray<float, 3> b_;
    RigidBodyPulses& rbp0_;
    RigidBodyPulses& rbp1_;
    const NormalImpulse& normal_impulse_;
    FixedArray<float, 3> p_;
    float stiction_coefficient_;
    float friction_coefficient_;
};

class TireContactInfo1: public ContactInfo {
public:
    TireContactInfo1(
        const FrictionContactInfo1& fci,
        RigidBody& rb,
        size_t tire_id,
        const FixedArray<float, 3>& v3,
        const FixedArray<float, 3>& n3,
        const PhysicsEngineConfig& cfg);
    void solve(float dt, float relaxation) override;
private:
    FrictionContactInfo1 fci_;
    RigidBody& rb_;
    PowerIntent P_;
    size_t tire_id_;
    FixedArray<float, 3> v3_;
    FixedArray<float, 3> n3_;
    const PhysicsEngineConfig& cfg_;
};

void solve_contacts(std::list<std::unique_ptr<ContactInfo>>& cis, float dt);

}
