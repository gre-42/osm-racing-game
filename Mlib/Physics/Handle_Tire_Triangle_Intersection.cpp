#include "Handle_Tire_Triangle_Intersection.hpp"
#include <Mlib/Physics/Misc/Rigid_Body.hpp>
#include <Mlib/Physics/Power_To_Force.hpp>

using namespace Mlib;

/**
 * solve(1/2*m*(v1^2-v0^2) = P*t, v1);
 * 
 * P == NAN => brake
 */
void accelerate_positive(
    RigidBody& rb,
    float power,
    const FixedArray<float, 3>& v3,
    const FixedArray<float, 3>& n3,
    float v0,
    const FixedArray<float, 3>& surface_normal,
    float force_n1,
    const PhysicsEngineConfig& cfg,
    size_t tire_id)
{
    // r = v / w
    float u = 1;
    if (float vc2 = sum(squared(v3)); vc2 > 1e-12) {
        if (-v0 > 1e-12) {
            u = std::clamp<float>(-v0 / std::sqrt(vc2), 1e-1, 1e1);
        }
    }
    float P = power;
    float m = rb.rbi_.rbp_.effective_mass({.vector = n3, .position = rb.get_abs_tire_contact_position(tire_id)});
    float dt = cfg.dt / cfg.oversampling;

    float v10 = -std::sqrt(squared(v0) + 2 * P * dt / m);
    float v11 = v0 - cfg.stiction_coefficient * force_n1 / m * dt;

    float vv = u * std::max(v10, v11);
    // std::cerr << P << " " << v10 << " " << v11 << " " << v0 << " " << force_n1 << " " << vv << std::endl;
    rb.set_tire_angular_velocity(tire_id, vv / rb.get_tire_radius(tire_id));
}

/**
 * solve(1/2*m*(v1^2-v0^2) = P*t, v1);
 * 
 * P == NAN => brake
 */
void accelerate_negative(
    RigidBody& rb,
    float power,
    const FixedArray<float, 3>& v3,
    const FixedArray<float, 3>& n3,
    float v0,
    const FixedArray<float, 3>& surface_normal,
    float force_n1,
    const PhysicsEngineConfig& cfg,
    size_t tire_id)
{
    // r = v / w
    float u = 1;
    if (float vc2 = sum(squared(v3)); vc2 > 1e-12) {
        if (v0 > 1e-12) {
            u = std::clamp<float>(v0 / std::sqrt(vc2), 1e-1, 1e1);
        }
    }
    float P = power;
    float m = rb.rbi_.rbp_.effective_mass({.vector = n3, .position = rb.get_abs_tire_contact_position(tire_id)});
    float dt = cfg.dt / cfg.oversampling;

    float v10 = std::sqrt(squared(v0) - 2 * P * dt / m);
    float v11 = v0 + cfg.stiction_coefficient * force_n1 / m * dt;

    float vv = u * std::min(v10, v11);
    rb.set_tire_angular_velocity(tire_id, vv / rb.get_tire_radius(tire_id));
}

void break_positive(
    RigidBody& rb,
    const FixedArray<float, 3>& v3,
    const FixedArray<float, 3>& n3,
    const FixedArray<float, 3>& surface_normal,
    float force_n1,
    const PhysicsEngineConfig& cfg,
    size_t tire_id)
{
    float w = rb.get_angular_velocity_at_tire(surface_normal, tire_id);
    rb.set_tire_angular_velocity(
        tire_id,
        std::max(w - 10.f, 0.f));
    // FixedArray<float, 3> tf0 = friction_force_infinite_mass(
    //     cfg.stiction_coefficient * force_n1,
    //     cfg.friction_coefficient * force_n1,
    //     v3,
    //     cfg.alpha0);
    // float f0 = dot0d(tf0, z3);
    // float v_max = 400.f / 3.6f;
    // float vv;
    // for (vv = v_max; vv >= 0; vv -= 0.1) {
    //     FixedArray<float, 3> tf = friction_force_infinite_mass(
    //         cfg.stiction_coefficient * force_n1,
    //         cfg.friction_coefficient * force_n1,
    //         v3 + n3 * vv,
    //         cfg.alpha0);
    //     if (dot0d(tf, z3) > f0 * 0.5f) {
    //         break;
    //     }
    // }
    // rb.set_tire_angular_velocity(tire_id, vv / rb.get_tire_radius(tire_id));
}

void break_negative(
    RigidBody& rb,
    const FixedArray<float, 3>& v3,
    const FixedArray<float, 3>& n3,
    const FixedArray<float, 3>& surface_normal,
    float force_n1,
    const PhysicsEngineConfig& cfg,
    size_t tire_id)
{
    float w = rb.get_angular_velocity_at_tire(surface_normal, tire_id);
    rb.set_tire_angular_velocity(
        tire_id,
        std::min(w + 10.f, 0.f));
    // FixedArray<float, 3> tf0 = friction_force_infinite_mass(
    //     cfg.stiction_coefficient * force_n1,
    //     cfg.friction_coefficient * force_n1,
    //     v3,
    //     cfg.alpha0);
    // float f0 = dot0d(tf0, n3);
    // float v_max = 400.f / 3.6f;
    // float vv;
    // for (vv = -v_max; vv <= 0; vv += 0.1) {
    //     FixedArray<float, 3> tf = friction_force_infinite_mass(
    //         cfg.stiction_coefficient * force_n1,
    //         cfg.friction_coefficient * force_n1,
    //         v3 + n3 * vv,
    //         cfg.alpha0);
    //     if (-dot0d(tf, n3) > -f0 * 0.5f) {
    //         break;
    //     }
    // }
    // rb.set_tire_angular_velocity(tire_id, vv / rb.get_tire_radius(tire_id));
}

void idle(RigidBody& rb, const FixedArray<float, 3>& surface_normal, size_t tire_id) {
    rb.set_tire_angular_velocity(tire_id, rb.get_angular_velocity_at_tire(surface_normal, tire_id));
}

FixedArray<float, 3> Mlib::updated_tire_speed(
    const PowerIntent& P,
    RigidBody& rb,
    const FixedArray<float, 3>& v3,
    const FixedArray<float, 3>& n3,
    float v0,
    float w0,
    const FixedArray<float, 3>& surface_normal,
    float force_n1,
    const PhysicsEngineConfig& cfg,
    size_t tire_id)
{
    // F = W / s = W / v / t = P / v
    if (!std::isnan(P.power)) {
        // std::cerr << "dx " << dx << std::endl;
        bool slipping = false;
        if ((P.power != 0) && !slipping) {
            float v = dot0d(rb.rbi_.rbp_.v_, n3);
            if (sign(P.power) != sign(v) && std::abs(v) > cfg.hand_break_velocity) {
                if (P.power > 0) {
                    break_positive(rb, v3, n3, surface_normal, force_n1, cfg, tire_id);
                } else if (P.power < 0) {
                    break_negative(rb, v3, n3, surface_normal, force_n1, cfg, tire_id);
                }
            } else if (P.power > 0) {
                if (P.type == PowerIntentType::BREAK_OR_IDLE) {
                    idle(rb, surface_normal, tire_id);
                } else {
                    accelerate_positive(rb, P.power, v3, n3, v0, surface_normal, force_n1, cfg, tire_id);
                }
            } else if (P.power < 0) {
                if (P.type == PowerIntentType::BREAK_OR_IDLE) {
                    idle(rb, surface_normal, tire_id);
                } else {
                    accelerate_negative(rb, P.power, v3, n3, v0, surface_normal, force_n1, cfg, tire_id);
                }
            }
        } else if (P.power == 0) {
            idle(rb, surface_normal, tire_id);
        }
    }
    float v1 = rb.get_tire_angular_velocity(tire_id) * rb.get_tire_radius(tire_id);
    return n3 * v1;
}

FixedArray<float, 3> Mlib::handle_tire_triangle_intersection(
    RigidBody& rb,
    const FixedArray<float, 3>& v3,
    const FixedArray<float, 3>& n3,
    const FixedArray<float, 3>& surface_normal,
    float force_n1,
    const PhysicsEngineConfig& cfg,
    size_t tire_id)
{
    return friction_force_infinite_mass(
        cfg.stiction_coefficient * force_n1,
        cfg.friction_coefficient * force_n1,
        v3 + updated_tire_speed(
            rb.consume_tire_surface_power(tire_id),
            rb,
            v3,
            n3,
            -dot0d(rb.get_velocity_at_tire_contact(surface_normal, tire_id), n3),
            rb.get_angular_velocity_at_tire(surface_normal, tire_id),
            surface_normal,
            force_n1,
            cfg,
            tire_id),
        cfg.alpha0);
}
