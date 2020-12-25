#include "Rigid_Body.hpp"
#include <Mlib/Geometry/Fixed_Cross.hpp>
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Math/Fixed_Rodrigues.hpp>
#include <Mlib/Math/Pi.hpp>
#include <Mlib/Physics/Interfaces/Damageable.hpp>
#include <Mlib/Physics/Misc/Rigid_Body_Engine.hpp>
#include <chrono>

using namespace Mlib;

RigidBody::RigidBody(RigidBodies& rigid_bodies, const RigidBodyIntegrator& rbi)
: rigid_bodies_{rigid_bodies},
  max_velocity_{INFINITY},
#ifdef COMPUTE_POWER
  power_{NAN},
  energy_old_{NAN},
#endif
  tires_z_{0, 0, -1},
  rbi_{rbi},
  damageable_{nullptr},
  driver_{nullptr}
{}

RigidBody::~RigidBody()
{}

void RigidBody::reset_forces() {
    rbi_.reset_forces();
    for (auto& e : engines_) {
        e.second.reset_forces();
    }
}

void RigidBody::integrate_force(const VectorAtPosition<float, 3>& F)
{
    rbi_.integrate_force(F);
}

void RigidBody::integrate_force(
    const VectorAtPosition<float, 3>& F,
    const FixedArray<float, 3>& n,
    float damping,
    float friction)
{
    integrate_force(F);
    if (damping != 0) {
        auto vn = n * dot0d(rbi_.rbp_.v_, n);
        auto vt = rbi_.rbp_.v_ - vn;
        rbi_.rbp_.v_ = (1 - damping) * vn + vt * (1 - friction);
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
    rbi_.integrate_gravity(g);
}

void RigidBody::advance_time(
    float dt,
    float min_acceleration,
    float min_velocity,
    float min_angular_velocity,
    PhysicsType physics_type,
    ResolveCollisionType resolve_collision_type,
    float hand_break_velocity,
    std::list<Beacon>* beacons)
{
    std::lock_guard lock{advance_time_mutex_};
    if (physics_type == PhysicsType::TRACKING_SPRINGS) {
        for (auto& t : tires_) {
            FixedArray<float, 3, 3> rotation = get_abs_tire_rotation_matrix(t.first);
            FixedArray<float, 3> position = get_abs_tire_contact_position(t.first);
            FixedArray<float, 3> power_axis = get_abs_tire_z(t.first);
            FixedArray<float, 3> velocity = velocity_at_position(position);
            float spring_constant = 1e4;
            float power_internal;
            float power_external;
            float moment;
            bool slipping;
            t.second.tracking_wheel.update_position(
                rotation,
                position,
                power_axis,
                velocity,
                spring_constant,
                dt,
                rbi_,
                power_internal,
                power_external,
                moment,
                slipping,
                beacons);
            // static float spower = 0;
            // spower = 0.99 * spower + 0.01 * power;
            // std::cerr << "rb force " << force << std::endl;
            float P = consume_tire_surface_power(t.first).power;
            // std::cerr << "P " << P << " Pi " << power_internal << " Pe " << power_external << " " << (P > power_internal) << std::endl;
            if (!std::isnan(P)) {
                float dx_max = 0.1;
                float w_max = dx_max / (t.second.tracking_wheel.radius() * dt);
                // std::cerr << "dx " << dx << std::endl;
                if ((P != 0) && (std::abs(P) > -power_internal) && !slipping) {
                    float v = dot0d(velocity, power_axis);
                    if (sign(P) != sign(v) && std::abs(v) > hand_break_velocity) {
                        t.second.tracking_wheel.set_w(0);
                    } else if (P > 0) {
                        t.second.tracking_wheel.set_w(std::max(-w_max, t.second.tracking_wheel.w() - 0.5f));
                    } else if (P < 0) {
                        t.second.tracking_wheel.set_w(std::min(w_max, t.second.tracking_wheel.w() + 0.5f));
                    }
                } else {
                    if (moment < 0) {
                        t.second.tracking_wheel.set_w(std::max(-w_max, t.second.tracking_wheel.w() - 0.5f));
                    } else if (moment > 0) {
                        t.second.tracking_wheel.set_w(std::min(w_max, t.second.tracking_wheel.w() + 0.5f));
                    }
                }
            }
        }
    }
    if (resolve_collision_type == ResolveCollisionType::PENALTY) {
        rbi_.advance_time(dt, min_acceleration, min_velocity, min_angular_velocity);
    } else {
        rbi_.rbp_.advance_time(dt);
    }
    for (auto& t : tires_) {
        t.second.advance_time(dt);
    }
#ifdef COMPUTE_POWER
    float nrg = energy();
    if (!std::isnan(energy_old_)) {
        power_ = (nrg - energy_old_) / dt;
    }
    energy_old_ = nrg;
#endif
}

float RigidBody::mass() const {
    return rbi_.rbp_.mass_;
}

FixedArray<float, 3> RigidBody::abs_com() const {
    return rbi_.rbp_.abs_com_;
}

FixedArray<float, 3, 3> RigidBody::abs_I() const {
    return rbi_.abs_I();
}

VectorAtPosition<float, 3> RigidBody::abs_F(const VectorAtPosition<float, 3>& F) const {
    return {
        vector: dot1d(rbi_.rbp_.rotation_, F.vector),
        position: dot1d(rbi_.rbp_.rotation_, F.position) + rbi_.rbp_.abs_com_};
}

FixedArray<float, 3> RigidBody::velocity_at_position(const FixedArray<float, 3>& position) const {
    return rbi_.velocity_at_position(position);
}

void RigidBody::set_absolute_model_matrix(const FixedArray<float, 4, 4>& absolute_model_matrix) {
    rbi_.set_pose(
        R3_from_4x4(absolute_model_matrix),
        t3_from_4x4(absolute_model_matrix));
}

FixedArray<float, 4, 4> RigidBody::get_new_absolute_model_matrix() const {
    std::lock_guard lock{advance_time_mutex_};
    return assemble_homogeneous_4x4(rbi_.rbp_.rotation_, rbi_.abs_position());
}

void RigidBody::notify_destroyed(void* obj) {
    rigid_bodies_.delete_rigid_body(this);
}

void RigidBody::set_max_velocity(float max_velocity) {
    max_velocity_ = max_velocity;
}

void RigidBody::set_tire_angle_y(size_t id, float angle_y) {
    tires_.at(id).angle_y = angle_y;
}

// void RigidBody::set_tire_accel_x(size_t id, float accel_x) {
//     tires_.at(id).accel_x = accel_x;
// }

FixedArray<float, 3, 3> RigidBody::get_abs_tire_rotation_matrix(size_t id) const {
    if (auto t = tires_.find(id); t != tires_.end()) {
        return dot2d(rbi_.rbp_.rotation_, rodrigues(FixedArray<float, 3>{0, 1, 0}, t->second.angle_y));
    } else {
        return rbi_.rbp_.rotation_;
    }
}

FixedArray<float, 3> RigidBody::get_abs_tire_z(size_t id) const {
    FixedArray<float, 3> z{tires_z_};
    if (auto t = tires_.find(id); t != tires_.end()) {
        z = dot1d(rodrigues(FixedArray<float, 3>{0, 1, 0}, t->second.angle_y), z);
    }
    z = dot1d(rbi_.rbp_.rotation_, z);
    return z;
}

float RigidBody::get_tire_angular_velocity(size_t id) const {
    return tires_.at(id).angular_velocity;
}

void RigidBody::set_tire_angular_velocity(size_t id, float w) {
    tires_.at(id).angular_velocity = w;
}

FixedArray<float, 3> RigidBody::get_velocity_at_tire_contact(const FixedArray<float, 3>& surface_normal, size_t id) const {
    FixedArray<float, 3> v = velocity_at_position(get_abs_tire_contact_position(id));
    v -= surface_normal * dot0d(v, surface_normal);
    return v;
}

float RigidBody::get_angular_velocity_at_tire(const FixedArray<float, 3>& surface_normal, size_t id) const {
    auto z = get_abs_tire_z(id);
    auto v = get_velocity_at_tire_contact(surface_normal, id);
    return -dot0d(v, z) / get_tire_radius(id);
}

float RigidBody::get_tire_radius(size_t id) const {
    return tires_.at(id).radius;
}

PowerIntent RigidBody::consume_tire_surface_power(size_t id) {
    auto en = tires_.find(id);
    if (en == tires_.end()) {
        return PowerIntent{.power = 0, .type = PowerIntentType::ALWAYS_IDLE};
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

float RigidBody::get_tire_break_force(size_t id) const {
    return tires_.at(id).break_force;
}

TrackingWheel& RigidBody::get_tire_tracking_wheel(size_t id) {
    return tires_.at(id).tracking_wheel;
}

FixedArray<float, 3> RigidBody::get_abs_tire_contact_position(size_t id) const {
    return rbi_.rbp_.transform_to_world_coordinates(tires_.at(id).position - FixedArray<float, 3>{0, -tires_.at(id).radius, 0});
}

float RigidBody::energy() const {
    return rbi_.energy();
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

void RigidBody::write_status(std::ostream& ostr, unsigned int log_components) const {
    if (log_components & STATUS_TIME) {
        static const std::chrono::steady_clock::time_point epoch_time = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point current_time = std::chrono::steady_clock::now();
        int64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - epoch_time).count();
        ostr << "t: " << milliseconds << " ms" << std::endl;
    }
    if (log_components & STATUS_SPEED) {
        ostr << "v: " << std::sqrt(sum(squared(rbi_.rbp_.v_))) * 3.6 << " km/h" << std::endl;
    }
    if (log_components & STATUS_ACCELERATION) {
        ostr << "a: " << std::sqrt(sum(squared(rbi_.a_))) << " m/s^2" << std::endl;
    }
    if (log_components & STATUS_DIAMETER) {
        // T = 2 PI r / v, T = 2 PI / w
        // r = v / w
        // r / r2 = v * a / (w * v^2) = a / (w * v)
        if (float w2 = sum(squared(rbi_.rbp_.w_)); w2 > 1e-2) {
            ostr << "d: " << 2 * std::sqrt(sum(squared(rbi_.rbp_.v_)) / w2) << " m" << std::endl;
            ostr << "d / d2(9.8): " << 9.8 / std::sqrt(w2 * sum(squared(rbi_.rbp_.v_))) << std::endl;
        } else {
            ostr << "d: " << " undefined" << std::endl;
            ostr << "d / d2(9.8): undefined" << std::endl;
        }
    }
    if (log_components & STATUS_DIAMETER2) {
        // F = m * a = m v^2 / r
        // r = v^2 / a
        if (float a2 = sum(squared(rbi_.a_)); a2 > 1e-2) {
            ostr << "d2: " << 2 * sum(squared(rbi_.rbp_.v_)) / std::sqrt(a2) << " m" << std::endl;
        } else {
            ostr << "d2: " << " undefined" << std::endl;
        }
        // Not implemented: https://de.wikipedia.org/wiki/Wendekreis_(Fahrzeug)
        // D = 2 L / sin(alpha)
    }
    if (log_components & STATUS_POSITION) {
        auto pos = rbi_.abs_position();
        ostr << "x: " << pos(0) << " m" << std::endl;
        ostr << "y: " << pos(1) << " m" << std::endl;
        ostr << "z: " << pos(2) << " m" << std::endl;
    }
    if (log_components & STATUS_ENERGY) {
        ostr << "E: " << energy() / 1e3 << " kJ" << std::endl;
#ifdef COMPUTE_POWER
        // ostr << "P: " << power_ / 1e3 << " W" << std::endl;
        if (!std::isnan(power_)) {
            ostr << "P: " << power_ * 0.00135962 << " PS" << std::endl;
        }
#endif
    }
    for (const auto& o : collision_observers_) {
        auto c = std::dynamic_pointer_cast<StatusWriter>(o);
        if (c != nullptr) {
            c->write_status(ostr, log_components);
        }
    }
    {
        auto c = dynamic_cast<StatusWriter*>(damageable_);
        if (c != nullptr) {
            c->write_status(ostr, log_components);
        }
    }
}
