#include "Constraints.hpp"
#include <Mlib/Geometry/Vector_At_Position.hpp>
#include <Mlib/Physics/Misc/Rigid_Body_Pulses.hpp>

using namespace Mlib;

/**
 * From: Erin Catto, Modeling and Solving Constraints
 *       Erin Catto, Fast and Simple Physics using Sequential Impulses
 *       Marijn Tamis, Giuseppe Maggiore, Constraint based physics solver
 *       Marijn Tamis, Sequential Impulse Solver for Rigid Body Dynamics
 */
void ContactInfo::solve(float dt, float beta, float beta2, float* lambda_total) {
    float lt = 0;
    if (pc.active(p)) {
        for(size_t j = 0; j < 1; ++j) {
            float v = dot0d(rbp.velocity_at_position(p), pc.plane.normal_);
            float mc = rbp.effective_mass({.vector = pc.plane.normal_, .position = p});
            float lambda = - mc * (-v + pc.b + 1.f / dt * (beta * pc.C(p) - beta2 * pc.bias(p)));
            lambda = std::clamp(lt + lambda, pc.lambda_min, pc.lambda_max) - lt;
            rbp.integrate_impulse({
                .vector = -pc.plane.normal_ * lambda,
                .position = p});
            // std::cerr << rbp.abs_position() << " | " << rbp.v_ << " | " << pc.active(x) << " | " << pc.overlap(x) << " | " << pc.bias(x) << std::endl;
        }
    }
    if (lambda_total != nullptr) {
        *lambda_total = lt;
    }
}

void Mlib::solve_contacts(std::list<ContactInfo>& cis, float dt, float beta, float beta2) {
    for(size_t i = 0; i < 10; ++i) {
        for(ContactInfo& ci : cis) {
            ci.solve(dt, beta, beta2);
        }
    }
}
