#include "Damageable.hpp"
#include <Mlib/Physics/Containers/Advance_Times.hpp>
#include <Mlib/Scene_Graph/Scene.hpp>

using namespace Mlib;

Damageable::Damageable(
    Scene& scene,
    AdvanceTimes& advance_times,
    const std::string& root_node_name,
    float health)
: scene_{scene},
  advance_times_{advance_times},
  root_node_name_{root_node_name},
  health_{health}
{
    scene_.get_node(root_node_name_)->add_destruction_observer(this);
}

void Damageable::notify_collided(
    const std::list<std::shared_ptr<CollisionObserver>>& collision_observers,
    CollisionType& collision_type,
    bool& abort)
{}

void Damageable::notify_destroyed(void* obj) {
    advance_times_.schedule_delete_advance_time(this);
}

void Damageable::advance_time(float dt) {
    if (health_ <= 0) {
        scene_.delete_root_node(root_node_name_);
    }
}

void Damageable::log(std::ostream& ostr, unsigned int log_components) const {
    if (log_components & LOG_HEALTH) {
        ostr << "HP: " << health_ << std::endl;
    }
}

void Damageable::damage(float amount) {
    health_ -= amount;
}
