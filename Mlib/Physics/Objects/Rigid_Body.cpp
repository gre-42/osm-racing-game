#include "Rigid_Body.hpp"
#include <Mlib/Geometry/Fixed_Cross.hpp>
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Math/Fixed_Rodrigues.hpp>
#include <Mlib/Math/Pi.hpp>
#include <Mlib/Physics/Objects/Rigid_Body_Engine.hpp>
#include <chrono>

using namespace Mlib;

RigidBody::RigidBody(
    RigidBodies& rigid_bodies,
    float mass,
    const FixedArray<float, 3>& L,    // angular momentum
    const FixedArray<float, 3, 3>& I, // inertia tensor
    const FixedArray<float, 3>& com,  // center of mass
    const FixedArray<float, 3>& v,    // velocity
    const FixedArray<float, 3>& w,    // angular velocity
    const FixedArray<float, 3>& T,    // torque
    const FixedArray<float, 3>& position,
    const FixedArray<float, 3>& rotation,
    bool I_is_diagonal)
: rigid_bodies_{rigid_bodies},
  max_velocity_{INFINITY},
  tires_z_{0, 0, -1},
  mass_{mass},
  rbi_{L,
       I,
       com,
       v,
       w,
       T,
       position,
       rotation,
       I_is_diagonal}
{}

RigidBody::~RigidBody()
{}

void RigidBody::reset_forces() {
    rbi_.a_ = 0.f;
    rbi_.T_ = 0.f;
    for(auto& e : engines_) {
        e.second.reset_forces();
    }
}

void RigidBody::integrate_force(const VectorAtPosition<float, 3>& F)
{
    rbi_.a_ += F.vector / mass_;
    rbi_.T_ += cross(F.position - abs_com(), F.vector);
}

void RigidBody::integrate_force(
    const VectorAtPosition<float, 3>& F,
    const FixedArray<float, 3>& n,
    float damping,
    float friction)
{
    integrate_force(F);
    if (damping != 0) {
        auto vn = n * dot0d(rbi_.v_, n);
        auto vt = rbi_.v_ - vn;
        rbi_.v_ = (1 - damping) * vn + vt * (1 - friction);
        rbi_.L_ *= 1 - damping;
    }
    if (damping != 0) {
        auto an = n * dot0d(rbi_.a_, n);
        auto at = rbi_.a_ - an;
        rbi_.a_ = (1 - damping) * an + at * (1 - friction);
        rbi_.T_ *= 1 - damping;
    }
}

void RigidBody::integrate_gravity(const FixedArray<float, 3>& g) {
    rbi_.a_ += g;
}

void RigidBody::advance_time(
    float dt,
    float min_acceleration,
    float min_velocity,
    float min_angular_velocity)
{
    std::lock_guard lock{advance_time_mutex_};
    rbi_.advance_time(dt, min_acceleration, min_velocity, min_angular_velocity);
    for(auto& t : tires_) {
        t.second.advance_time(dt);
    }
}

float RigidBody::mass() const {
    return mass_;
}

FixedArray<float, 3> RigidBody::abs_com() const {
    return rbi_.abs_com_;
}

FixedArray<float, 3, 3> RigidBody::abs_I() const {
    return rbi_.abs_I();
}

VectorAtPosition<float, 3> RigidBody::abs_F(const VectorAtPosition<float, 3>& F) const {
    return {
        vector: dot1d(rbi_.rotation_, F.vector),
        position: dot1d(rbi_.rotation_, F.position) + rbi_.abs_com_};
}

FixedArray<float, 3> RigidBody::velocity_at_position(const FixedArray<float, 3>& position) const {
    return rbi_.velocity_at_position(position);
}

void RigidBody::set_absolute_model_matrix(const FixedArray<float, 4, 4>& absolute_model_matrix) {
    rbi_.rotation_ = R3_from_4x4(absolute_model_matrix);
    rbi_.abs_com_ = dot1d(rbi_.rotation_, rbi_.com_) + t3_from_4x4(absolute_model_matrix);
}

FixedArray<float, 4, 4> RigidBody::get_new_absolute_model_matrix() const {
    std::lock_guard lock{advance_time_mutex_};
    return assemble_homogeneous_4x4(rbi_.rotation_, rbi_.abs_position());
}

void RigidBody::notify_destroyed(void* obj) {
    rigid_bodies_.delete_rigid_body(this);
}

void RigidBody::set_max_velocity(float max_velocity) {
    max_velocity_ = max_velocity;
}

void RigidBody::set_tire_angle(size_t id, float angle) {
    tires_.at(id).angle = angle;
}

FixedArray<float, 3> RigidBody::get_abs_tire_z(size_t id) const {
    auto t = tires_.find(id);
    FixedArray<float, 3> z{tires_z_};
    if (t != tires_.end()) {
        z = dot1d(rodrigues(FixedArray<float, 3>{0, 1, 0}, t->second.angle / 180.f * float(M_PI)), z);
    }
    z = dot1d(rbi_.rotation_, z);
    return z;
}

float RigidBody::consume_tire_surface_power(size_t id) {
    auto en = tires_.find(id);
    if (en == tires_.end()) {
        return 0;
    }
    auto e = engines_.find(en->second.engine);
    if (e == engines_.end()) {
        throw std::runtime_error("No engine with name \"" + en->second.engine + "\" exists");
    }
    return e->second.consume_abs_surface_power();
}

void RigidBody::set_surface_power(const std::string& engine_name, float surface_power) {
    auto e = engines_.find(engine_name);
    if (e == engines_.end()) {
        throw std::runtime_error("No engine with name \"" + engine_name + "\" exists");
    }
    e->second.set_surface_power(surface_power);
}

// void RigidBody::set_tire_sliding(size_t id, bool value) {
//     tire_sliding_[id] = value;
// }
// bool RigidBody::get_tire_sliding(size_t id) const {
//     auto t = tire_sliding_.find(id);
//     if (t != tire_sliding_.end()) {
//         return t->second;
//     }
//     return false;
// }

void RigidBody::log(std::ostream& ostr, unsigned int log_components) const {
    if (log_components & LOG_TIME) {
        static const std::chrono::steady_clock::time_point epoch_time = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
        int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - epoch_time).count();
        ostr << "t: " << milliseconds << " ms" << std::endl;
    }
    if (log_components & LOG_SPEED) {
        ostr << "v: " << std::sqrt(sum(squared(rbi_.v_))) * 3.6 << " km/h" << std::endl;
    }
    if (log_components & LOG_ACCELERATION) {
        ostr << "a: " << std::sqrt(sum(squared(rbi_.a_))) << " m/s^2" << std::endl;
    }
    if (log_components & LOG_DIAMETER) {
        // T = 2 PI R / v, T = 2 PI / w
        if (float w2 = sum(squared(rbi_.w_)); w2 > 1e-2) {
            ostr << "d: " << 2 * std::sqrt(sum(squared(rbi_.v_)) / w2) << " m" << std::endl;
        } else {
            ostr << "d: " << " undefined" << std::endl;
        }
    }
    if (log_components & LOG_DIAMETER2) {
        // F = m*a = mv^2/r
        // r = v^2/a
        if (float a2 = sum(squared(rbi_.a_)); a2 > 1e-2) {
            ostr << "d2: " << 2 * sum(squared(rbi_.v_)) / std::sqrt(a2) << " m" << std::endl;
        } else {
            ostr << "d2: " << " undefined" << std::endl;
        }
    }
    if (log_components & LOG_POSITION) {
        auto pos = rbi_.abs_position();
        ostr << "x: " << pos(0) << " m" << std::endl;
        ostr << "y: " << pos(1) << " m" << std::endl;
        ostr << "z: " << pos(2) << " m" << std::endl;
    }
    for(const auto& o : collision_observers_) {
        auto c = std::dynamic_pointer_cast<Loggable>(o);
        if (c != nullptr) {
            c->log(ostr, log_components);
        }
    }
}
