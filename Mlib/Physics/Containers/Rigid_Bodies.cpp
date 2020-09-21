#include "Rigid_Bodies.hpp"
#include <Mlib/Geometry/Fixed_Cross.hpp>
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Geometry/Mesh/Colored_Vertex_Array.hpp>
#include <Mlib/Physics/Objects/Rigid_Body.hpp>
#include <Mlib/Physics/Transformed_Mesh.hpp>

using namespace Mlib;

RigidBodies::RigidBodies(const PhysicsEngineConfig& cfg)
: cfg_{cfg}
{}

std::list<std::vector<CollisionTriangle>> split_with_static_radius(
    const std::list<std::shared_ptr<ColoredVertexArray>>& cvas,
    const FixedArray<float, 4, 4>& tm,
    float static_radius)
{
    if (std::isnan(static_radius)) {
        throw std::runtime_error("Static objects require a non-NAN static_radius");
    }
    if (any(isnan(tm))) {
        throw std::runtime_error("Transformation matrix contains NAN values. Forgot to add rigid body to scene node?");
    }
    std::list<std::pair<FixedArray<float, 3>, std::list<CollisionTriangle>>> centers;
    for(auto& m : cvas) {
        if (m->material.collide) {
            for(const auto& t : m->transformed_triangles(tm)) {
                bool sphere_found = false;
                for(auto& x : centers) {
                    if (sum(squared(t.bounding_sphere.center() - x.first)) < squared(static_radius)) {
                        x.second.push_back(t);
                        sphere_found = true;
                        break;
                    }
                }
                if (!sphere_found) {
                    centers.push_back(std::make_pair(t.bounding_sphere.center(), std::list{t}));
                }
            }
        }
    }
    std::list<std::vector<CollisionTriangle>> result;
    for(const auto& x : centers) {
        std::vector<CollisionTriangle> res{x.second.begin(), x.second.end()};
        result.push_back(std::move(res));
    }
    return result;
}

void RigidBodies::add_rigid_body(
    const std::shared_ptr<RigidBody>& rigid_body,
    const std::list<std::shared_ptr<ColoredVertexArray>>& hitbox,
    const std::list<std::shared_ptr<ColoredVertexArray>>& tirelines)
{
    if (rigid_body->mass() == INFINITY) {
        if (!tirelines.empty()) {
            throw std::runtime_error("static rigid body has tirelines");
        }
        auto xx = split_with_static_radius(hitbox, rigid_body->get_new_absolute_model_matrix(), cfg_.static_radius);
        RigidBodyAndTransformedMeshes rbtm;
        rbtm.rigid_body = rigid_body;
        
        for(const auto& p : xx) {
            std::vector<FixedArray<float, 3>> vertices;
            vertices.reserve(p.size() * 3);
            for(const auto& t : p) {
                vertices.push_back(t.triangle(0));
                vertices.push_back(t.triangle(1));
                vertices.push_back(t.triangle(2));
            }
            if (vertices.size() > 0) {
                BoundingSphere<float, 3> bs{vertices.begin(), vertices.end()};
                rbtm.meshes.push_back({
                    mesh_type: MeshType::CHASSIS,
                    mesh: std::make_shared<TransformedMesh>(bs, p)});
            }
        }
        
        transformed_objects_.push_back(rbtm);
    } else {
        RigidBodyAndMeshes rbm;
        rbm.rigid_body = rigid_body;
        auto ins = [this, &rbm](const auto& cvas, MeshType mesh_type) {
            for(auto& cva : cvas) {
                if (cva->material.collide) {
                    auto vertices = cva->vertices();
                    if (vertices.size() > 0) {
                        BoundingSphere<float, 3> bs{vertices.begin(), vertices.end()};
                        rbm.meshes.push_back({
                            mesh_type: mesh_type,
                            mesh: std::make_pair(bs, cva)});
                    }
                }
            }
        };
        ins(hitbox, MeshType::CHASSIS);
        ins(tirelines, MeshType::TIRE_LINE);
        objects_.push_back(std::move(rbm));
    }
}

void RigidBodies::delete_rigid_body(const RigidBody* rigid_body) {
    if (rigid_body->mass() == INFINITY) {
        auto it = std::find_if(transformed_objects_.begin(), transformed_objects_.end(), [rigid_body](const auto& e){ return e.rigid_body.get() == rigid_body; });
        if (it == transformed_objects_.end()) {
            throw std::runtime_error("Could not delete rigid body");
        }
        transformed_objects_.erase(it);
    } else {
        auto it = std::find_if(objects_.begin(), objects_.end(), [rigid_body](const auto& e){ return e.rigid_body.get() == rigid_body; });
        if (it == objects_.end()) {
            throw std::runtime_error("Could not delete rigid body");
        }
        objects_.erase(it);
    }
}
