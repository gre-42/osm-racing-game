#pragma once
#include <Mlib/Memory/Dangling_Unique_Ptr.hpp>
#include <Mlib/Memory/Destruction_Functions.hpp>
#include <Mlib/Scene_Graph/Animation/Animation_State_Updater.hpp>
#include <string>

namespace Mlib {

class RigidBodyVehicle;
class SceneNode;

class AvatarAnimationUpdater: public AnimationStateUpdater {
public:
    explicit AvatarAnimationUpdater(
        const RigidBodyVehicle& rb,
        DanglingRef<SceneNode> gun_node,
        const std::string& resource_wo_gun,
        const std::string& resource_w_gun);
    virtual ~AvatarAnimationUpdater() override;
    virtual void notify_movement_intent() override;
    virtual std::unique_ptr<AnimationState> update_animation_state(
        const AnimationState& animation_state) override;
private:
    const RigidBodyVehicle& rb_;
    DanglingPtr<SceneNode> gun_node_;
    std::string resource_wo_gun_;
    std::string resource_w_gun_;
    float surface_power_;
    DestructionFunctionsRemovalTokens gun_node_on_destroy_;
};

}
