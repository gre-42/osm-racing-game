#pragma once
#include <Mlib/Array/Fixed_Array.hpp>
#include <Mlib/Geometry/Plane_Nd.hpp>
#include <Mlib/Physics/Misc/Rigid_Body_Engine.hpp>
#include <iosfwd>
#include <list>

namespace Mlib {

class RigidBody;
class RigidBodyPulses;
struct PhysicsEngineConfig;

struct NormalImpulse {
    FixedArray<float, 3> normal;
    float lambda_total = 0;
};

struct PlaneEqualityConstraint {
    NormalImpulse normal_impulse;
    float intercept;
    float b = 0;
    bool always_active = true;
    float beta = 0.5;
    inline float C(const FixedArray<float, 3>& x) const {
        return -(dot0d(normal_impulse.normal, x) + intercept);
    }
    inline float v(const FixedArray<float, 3>& p, float dt) const {
        return b + beta / dt * C(p);
    }
};

struct PlaneInequalityConstraint {
    NormalImpulse normal_impulse;
    float intercept;
    float b = 0;
    float slop = 0;
    bool always_active = true;
    float beta = 0.02f;
    inline float C(const FixedArray<float, 3>& x) const {
        return -(dot0d(normal_impulse.normal, x) + intercept);
    }
    inline float overlap(const FixedArray<float, 3>& x) const {
        // std::cerr << plane.normal << " | " << x << " | " << plane.intercept << std::endl;
        return -(dot0d(normal_impulse.normal, x) + intercept);
    }
    inline float active(const FixedArray<float, 3>& x) const {
        return always_active || (overlap(x) > 0);
    }
    inline float bias(const FixedArray<float, 3>& x) const {
        return std::max(0.f, overlap(x) - slop);
    }
    inline float v(const FixedArray<float, 3>& p, float dt) const {
        return b + beta / dt * bias(p);
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

typedef BoundedConstraint1D<PlaneInequalityConstraint> BoundedPlaneInequalityConstraint;
typedef BoundedConstraint1D<ShockAbsorberConstraint> BoundedShockAbsorberConstraint;

class ContactInfo {
public:
    virtual void solve(float dt, float relaxation) = 0;
    virtual void finalize() {}
    virtual ~ContactInfo() = default;
};

class NormalContactInfo1: public ContactInfo {
public:
    NormalContactInfo1(
        RigidBodyPulses& rbp,
        const BoundedPlaneInequalityConstraint& pc,
        const FixedArray<float, 3>& p);
    void solve(float dt, float relaxation) override;
    const NormalImpulse& normal_impulse() const {
        return pc_.constraint.normal_impulse;
    }
private:
    RigidBodyPulses& rbp_;
    BoundedPlaneInequalityConstraint pc_;
    FixedArray<float, 3> p_;
};

class NormalContactInfo2: public ContactInfo {
public:
    NormalContactInfo2(
        RigidBodyPulses& rbp0,
        RigidBodyPulses& rbp1,
        const BoundedPlaneInequalityConstraint& pc,
        const FixedArray<float, 3>& p,
        std::function<void(float)> notify_lambda_final = [](float){});
    void solve(float dt, float relaxation) override;
    void finalize() override;
    const NormalImpulse& normal_impulse() const {
        return pc_.constraint.normal_impulse;
    }
private:
    RigidBodyPulses& rbp0_;
    RigidBodyPulses& rbp1_;
    BoundedPlaneInequalityConstraint pc_;
    FixedArray<float, 3> p_;
    std::function<void(float)> notify_lambda_final_;
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
    friend std::ostream& operator << (std::ostream& ostr, const FrictionContactInfo1& fci1);
public:
    FrictionContactInfo1(
        RigidBodyPulses& rbp,
        const NormalImpulse& normal_impulse,
        const FixedArray<float, 3>& p,
        float stiction_coefficient,
        float friction_coefficient,
        const FixedArray<float, 3>& b,
        const FixedArray<float, 3>& clamping_direction = fixed_nans<float, 3>(),
        float clamping_min = NAN,
        float clamping_max = NAN,
        float ortho_clamping_max_l2 = NAN,
        float extra_stiction = 0,
        float extra_friction = 0,
        float extra_w = 0);
    void solve(float dt, float relaxation) override;
    float max_impulse_stiction() const;
    float max_impulse_friction() const;
    const FixedArray<float, 3>& get_b() const;
    void set_b(const FixedArray<float, 3>& b);
    void set_clamping(
        const FixedArray<float, 3>& clamping_direction,
        float clamping_min,
        float clamping_max,
        float ortho_clamping_max_l2_);
    void set_extras(
        float extra_stiction,
        float extra_friction,
        float extra_w);
    const NormalImpulse& normal_impulse() {
        return normal_impulse_;
    }
    const FixedArray<float, 3>& lambda_total() {
        return lambda_total_;
    }
private:
    FixedArray<float, 3> lambda_total_;
    FixedArray<float, 3> b_;
    RigidBodyPulses& rbp_;
    const NormalImpulse& normal_impulse_;
    FixedArray<float, 3> p_;
    float stiction_coefficient_;
    float friction_coefficient_;
    FixedArray<float, 3> clamping_direction_;
    float clamping_min_;
    float clamping_max_;
    float ortho_clamping_max_l2_;
    float extra_stiction_;
    float extra_friction_;
    float extra_w_;
};

std::ostream& operator << (std::ostream& ostr, const FrictionContactInfo1& fci1);

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
    float max_impulse_stiction() const;
    float max_impulse_friction() const;
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
        const FixedArray<float, 3>& vc,
        const FixedArray<float, 3>& n3,
        float v0,
        const PhysicsEngineConfig& cfg);
    void solve(float dt, float relaxation) override;
    void finalize() override;
private:
    FrictionContactInfo1 fci_;
    RigidBody& rb_;
    PowerIntent P_;
    size_t tire_id_;
    FixedArray<float, 3> vc_;
    FixedArray<float, 3> n3_;
    float v0_;
    FixedArray<float, 3> b0_;
    const PhysicsEngineConfig& cfg_;
};

void solve_contacts(std::list<std::unique_ptr<ContactInfo>>& cis, float dt);

}
