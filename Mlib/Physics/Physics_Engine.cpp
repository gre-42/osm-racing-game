#include "Physics_Engine.hpp"
#include <Mlib/Math/Orderable_Fixed_Array.hpp>
#include <Mlib/Physics/Collision/Constraints.hpp>
#include <Mlib/Physics/Collision/Handle_Line_Triangle_Intersection.hpp>
#include <Mlib/Physics/Collision/Sat_Normals.hpp>
#include <Mlib/Physics/Collision/Transformed_Mesh.hpp>
#include <Mlib/Physics/Interfaces/Advance_Time.hpp>
#include <Mlib/Physics/Interfaces/External_Force_Provider.hpp>
#include <Mlib/Physics/Misc/Beacon.hpp>
#include <Mlib/Physics/Misc/Rigid_Body.hpp>
#include <Mlib/Reverse_Iterator.hpp>

using namespace Mlib;

PhysicsEngine::PhysicsEngine(
    const PhysicsEngineConfig& cfg,
    bool check_objects_deleted_on_destruction)
: rigid_bodies_{cfg},
  collision_query_{*this},
  cfg_{cfg},
  check_objects_deleted_on_destruction_{check_objects_deleted_on_destruction}
{}

PhysicsEngine::~PhysicsEngine() {
    // The physics thread calls "delete_scheduled_advance_times".
    // However, scene destruction (which schedules deletion) happens after physics thread joining
    // and before PhysicsEngine destruction.
    // => We need to call "delete_scheduled_advance_times" in the PhysicsEngine destructor (in the main thread).
    // No special handling is required for objects (i.e. rigid bodies), because their deletion is not scheduled,
    // but happens instantaneously.
    advance_times_.delete_scheduled_advance_times();
    if (check_objects_deleted_on_destruction_) {
        if (!rigid_bodies_.objects_.empty()) {
            std::cerr << "~PhysicsEngine: " << rigid_bodies_.objects_.size() << " objects still exist." << std::endl;
        }
        if (!advance_times_.advance_times_to_delete_.empty()) {
            std::cerr << "~PhysicsEngine: " << advance_times_.advance_times_to_delete_.size() << " advance_times_to_delete still exist." << std::endl;
        }
        if (!advance_times_.advance_times_shared_.empty()) {
            std::cerr << "~PhysicsEngine: " << advance_times_.advance_times_shared_.size() << " advance_times_shared still exist." << std::endl;
        }
        if (!advance_times_.advance_times_ptr_.empty()) {
            std::cerr << "~PhysicsEngine: " << advance_times_.advance_times_ptr_.size() << " advance_times_ptr still exist." << std::endl;
        }
    }
}

static void handle_triangle_triangle_intersection(
    RigidBody& o0,
    RigidBody& o1,
    const CollisionTriangleSphere& t0,
    const TypedMesh<std::shared_ptr<TransformedMesh>>& msh0,
    const TypedMesh<std::shared_ptr<TransformedMesh>>& msh1,
    std::list<Beacon>* beacons,
    std::list<std::unique_ptr<ContactInfo>>& contact_infos,
    const PhysicsEngineConfig& cfg,
    const SatTracker& st,
    BaseLog* base_log)
{
    for (const auto& t1 : msh1.mesh->get_triangles_sphere()) {
        if (!t1.bounding_sphere.intersects(t0.bounding_sphere)) {
            continue;
        }
        if (!t1.bounding_sphere.intersects(t0.plane)) {
            continue;
        }
        // Closed, triangulated surfaces contain every edge twice.
        // => Remove duplicates by checking the order.
        if (OrderableFixedArray{t1.triangle(1)} < OrderableFixedArray{t1.triangle(2)}) {
            handle_line_triangle_intersection({
                .o0 = o0,
                .o1 = o1,
                .mesh0 = msh0.mesh,
                .mesh1 = msh1.mesh,
                .l1 = FixedArray<FixedArray<float, 3>, 2>{t1.triangle(1), t1.triangle(2)},
                .t0 = t0.triangle,
                .p0 = t0.plane,
                .cfg = cfg,
                .st = st,
                .beacons = beacons,
                .contact_infos = contact_infos,
                .tire_id = SIZE_MAX,
                .mesh0_two_sided = t0.two_sided,
                .lines_are_normals = false,
                .base_log = base_log});
        }
        if (OrderableFixedArray{t1.triangle(2)} < OrderableFixedArray{t1.triangle(0)}) {
            handle_line_triangle_intersection({
                .o0 = o0,
                .o1 = o1,
                .mesh0 = msh0.mesh,
                .mesh1 = msh1.mesh,
                .l1 = FixedArray<FixedArray<float, 3>, 2>{t1.triangle(2), t1.triangle(0)},
                .t0 = t0.triangle,
                .p0 = t0.plane,
                .cfg = cfg,
                .st = st,
                .beacons = beacons,
                .contact_infos = contact_infos,
                .tire_id = SIZE_MAX,
                .mesh0_two_sided = t0.two_sided,
                .lines_are_normals = false,
                .base_log = base_log});
        }
        if (OrderableFixedArray{t1.triangle(0)} < OrderableFixedArray{t1.triangle(1)}) {
            handle_line_triangle_intersection({
                .o0 = o0,
                .o1 = o1,
                .mesh0 = msh0.mesh,
                .mesh1 = msh1.mesh,
                .l1 = FixedArray<FixedArray<float, 3>, 2>{t1.triangle(0), t1.triangle(1)},
                .t0 = t0.triangle,
                .p0 = t0.plane,
                .cfg = cfg,
                .st = st,
                .beacons = beacons,
                .contact_infos = contact_infos,
                .tire_id = SIZE_MAX,
                .mesh0_two_sided = t0.two_sided,
                .lines_are_normals = false,
                .base_log = base_log});
        }
    }
}

static void collide_triangle(
    RigidBody& o0,
    RigidBody& o1,
    const TypedMesh<std::shared_ptr<TransformedMesh>>& msh0,
    const TypedMesh<std::shared_ptr<TransformedMesh>>& msh1,
    const CollisionTriangleSphere& t0,
    const PhysicsEngineConfig& cfg,
    const SatTracker& st,
    std::list<Beacon>* beacons,
    std::list<std::unique_ptr<ContactInfo>>& contact_infos,
    BaseLog* base_log)
{
    // Mesh-sphere <-> triangle-sphere intersection
    if (!msh1.mesh->intersects(t0.bounding_sphere)) {
        return;
    }
    // Mesh-sphere <-> triangle-plane intersection
    if (!msh1.mesh->intersects(t0.plane)) {
        return;
    }
    if (!cfg.collide_only_normals && (o0.mass() != INFINITY)) {
        handle_triangle_triangle_intersection(
            o0,
            o1,
            t0,
            msh0,
            msh1,
            beacons,
            contact_infos,
            cfg,
            st,
            base_log);
    }
    const auto& lines = msh1.mesh->get_lines();
    if (msh1.mesh_type == MeshType::CHASSIS) {
        for (const auto& l1 : lines) {
            handle_line_triangle_intersection({
                .o0 = o0,
                .o1 = o1,
                .mesh0 = msh0.mesh,
                .mesh1 = msh1.mesh,
                .l1 = l1,
                .t0 = t0.triangle,
                .p0 = t0.plane,
                .cfg = cfg,
                .st = st,
                .beacons = beacons,
                .contact_infos = contact_infos,
                .tire_id = SIZE_MAX,
                .mesh0_two_sided = t0.two_sided,
                .lines_are_normals = true,
                .base_log = base_log});
        }
    } else if (msh1.mesh_type == MeshType::TIRE_LINE) {
        size_t tire_id = 0;
        for (const auto& l1 : lines) {
            handle_line_triangle_intersection({
                .o0 = o0,
                .o1 = o1,
                .mesh0 = msh0.mesh,
                .mesh1 = msh1.mesh,
                .l1 = l1,
                .t0 = t0.triangle,
                .p0 = t0.plane,
                .cfg = cfg,
                .st = st,
                .beacons = beacons,
                .contact_infos = contact_infos,
                .tire_id = tire_id,
                .mesh0_two_sided = t0.two_sided,
                .lines_are_normals = true,
                .base_log = base_log});
            ++tire_id;
        }
    } else {
        throw std::runtime_error("Unknown mesh type");
    }
}

static void collide_objects(
    const RigidBodyAndTransformedMeshes& o0,
    const RigidBodyAndTransformedMeshes& o1,
    const PhysicsEngineConfig& cfg,
    const SatTracker& st,
    std::list<Beacon>* beacons,
    std::list<std::unique_ptr<ContactInfo>>& contact_infos,
    BaseLog* base_log)
{
    if (o0.rigid_body == o1.rigid_body) {
        return;
    }
    if ((o0.rigid_body->mass() == INFINITY) && (o1.rigid_body->mass() == INFINITY)) {
        return;
    }
    if (o1.rigid_body->mass() == INFINITY) {
        return;
    }
    for (const auto& msh1 : o1.meshes) {
        for (const auto& msh0 : o0.meshes) {
            if (msh0.mesh_type == MeshType::TIRE_LINE) {
                continue;
            }
            if (!msh0.mesh->intersects(*msh1.mesh)) {
                continue;
            }
            for (const auto& t0 : msh0.mesh->get_triangles_sphere()) {
                collide_triangle(
                    *o0.rigid_body,
                    *o1.rigid_body,
                    msh0,
                    msh1,
                    t0,
                    cfg,
                    st,
                    beacons,
                    contact_infos,
                    base_log);
            }
        }
    }
}

void PhysicsEngine::collide(
    std::list<Beacon>* beacons,
    std::list<std::unique_ptr<ContactInfo>>& contact_infos,
    bool burn_in,
    BaseLog* base_log)
{
    std::erase_if(rigid_bodies_.transformed_objects_, [](const RigidBodyAndTransformedMeshes& rbtm){
        return (rbtm.rigid_body->mass() != INFINITY);
    });
    {
        std::list<std::shared_ptr<RigidBody>> olist;
        for (const auto& o : rigid_bodies_.objects_) {
            if (o.rigid_body->mass() != INFINITY) {
                o.rigid_body->reset_forces();
                olist.push_back(o.rigid_body);
            }
        }
        for (const auto& efp : external_force_providers_) {
            efp->increment_external_forces(olist, burn_in, cfg_);
        }
    }
    for (const auto& o : rigid_bodies_.objects_) {
        if (o.rigid_body->mass() != INFINITY) {
            if (o.meshes.size() == 0) {
                std::cerr << "Skipping object without meshes" << std::endl;
            }
            rigid_bodies_.transform_object_and_add(o);
        }
    }
    SatTracker st;
    collide_forward_ = !collide_forward_;
    if (collide_forward_) {
        for (const auto& o0 : rigid_bodies_.transformed_objects_) {
            for (const auto& o1 : rigid_bodies_.transformed_objects_) {
                collide_objects(o0, o1, cfg_, st, beacons, contact_infos, base_log);
            }
        }
    } else {
        for (const auto& o0 : reverse(rigid_bodies_.transformed_objects_)) {
            for (const auto& o1 : reverse(rigid_bodies_.transformed_objects_)) {
                collide_objects(o0, o1, cfg_, st, beacons, contact_infos, base_log);
            }
        }
    }
    if (cfg_.bvh) {
        static TypedMesh<std::shared_ptr<TransformedMesh>> o0_mesh;
        for (const auto& o1 : rigid_bodies_.transformed_objects_) {
            if (o1.rigid_body->mass() == INFINITY) {
                continue;
            }
            for (const auto& msh1 : o1.meshes) {
                rigid_bodies_.bvh_.visit(
                    msh1.mesh->transformed_bounding_sphere(),
                    [&](const RigidBodyAndCollisionTriangleSphere& t0){
                        collide_triangle(
                            t0.rb,
                            *o1.rigid_body,
                            o0_mesh,
                            msh1,
                            t0.ctp,
                            cfg_,
                            st,
                            beacons,
                            contact_infos,
                            base_log);
                    });
            }
        }
    }
}

void PhysicsEngine::move_rigid_bodies(std::list<Beacon>* beacons) {
    for (auto it = rigid_bodies_.objects_.begin(); it != rigid_bodies_.objects_.end(); ) {
        auto& o = *it++;
        assert (o.rigid_body->mass() != INFINITY);
        o.rigid_body->advance_time(
            cfg_.dt / cfg_.oversampling,
            cfg_.min_acceleration,
            cfg_.min_velocity,
            cfg_.min_angular_velocity,
            cfg_.physics_type,
            cfg_.resolve_collision_type,
            cfg_.hand_break_velocity,
            beacons);
    }
}

void PhysicsEngine::move_advance_times() {
    for (auto a : advance_times_.advance_times_shared_) {
        if (!advance_times_.advance_times_to_delete_.contains(a.get())) {
            a->advance_time(cfg_.dt);
        }
    }
    for (auto a : advance_times_.advance_times_ptr_) {
        if (!advance_times_.advance_times_to_delete_.contains(a)) {
            a->advance_time(cfg_.dt);
        }
    }
}

void PhysicsEngine::burn_in(float seconds) {
    for (const auto& o : rigid_bodies_.objects_) {
        for (auto& e : o.rigid_body->engines_) {
            e.second.set_surface_power(NAN);
        }
    }
    for (float time = 0; time < seconds; time += cfg_.dt / cfg_.oversampling) {
        {
            std::list<std::unique_ptr<ContactInfo>> contact_infos;
            collide(
                nullptr,        // beacons
                contact_infos,
                true,           // true = burn_in
                nullptr);       // base_log
            if (cfg_.resolve_collision_type == ResolveCollisionType::SEQUENTIAL_PULSES) {
                solve_contacts(contact_infos, cfg_.dt / cfg_.oversampling);
            }
        }
        if (time < seconds / 2) {
            for (const auto& o : rigid_bodies_.objects_) {
                o.rigid_body->rbi_.T_ = 0;
                if (cfg_.resolve_collision_type == ResolveCollisionType::SEQUENTIAL_PULSES) {
                    o.rigid_body->rbi_.rbp_.w_ = 0;
                }
            }
        }
        move_rigid_bodies(nullptr);
    }
}

void PhysicsEngine::add_external_force_provider(ExternalForceProvider* efp)
{
    external_force_providers_.push_back(efp);
}
