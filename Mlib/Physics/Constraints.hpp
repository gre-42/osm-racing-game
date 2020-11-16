#pragma once
#include <Mlib/Array/Fixed_Array.hpp>
#include <Mlib/Geometry/Plane_Nd.hpp>
#include <list>

namespace Mlib {

struct RigidBodyPulses;

struct PlaneConstraint {
    PlaneNd<float, 3> plane;
    float b;
    float slop;
    float lambda_min = -INFINITY;
    float lambda_max = INFINITY;
    bool always_active = false;
    float beta;
    float beta2;
    inline float C(const FixedArray<float, 3>& x) const {
        return -(dot0d(plane.normal_, x) + plane.intercept_);
    }
    inline float overlap(const FixedArray<float, 3>& x) const {
        // std::cerr << plane.normal_ << " | " << x << " | " << plane.intercept_ << std::endl;
        return -(dot0d(plane.normal_, x) + plane.intercept_);
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

class ContactInfo {
public:
    virtual void solve(float dt, float* lambda_total = nullptr) const = 0;
    virtual ~ContactInfo() = default;
};

class ContactInfo1: public ContactInfo {
public:
    ContactInfo1(
        RigidBodyPulses& rbp,
        const PlaneConstraint& pc,
        const FixedArray<float, 3>& p)
    : rbp{rbp},
      pc{pc},
      p{p}
    {}
    void solve(float dt, float* lambda_total = nullptr) const override;
private:
    RigidBodyPulses& rbp;
    PlaneConstraint pc;
    FixedArray<float, 3> p;
};

class ContactInfo2: public ContactInfo {
public:
    ContactInfo2(
        RigidBodyPulses& rbp0,
        RigidBodyPulses& rbp1,
        const PlaneConstraint& pc,
        const FixedArray<float, 3>& p)
    : rbp0{rbp0},
      rbp1{rbp1},
      pc{pc},
      p{p}
    {}
    void solve(float dt, float* lambda_total = nullptr) const override;
private:
    RigidBodyPulses& rbp0;
    RigidBodyPulses& rbp1;
    PlaneConstraint pc;
    FixedArray<float, 3> p;
};

void solve_contacts(std::list<std::unique_ptr<ContactInfo>>& cis, float dt);

}
