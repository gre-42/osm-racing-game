#include "Set_Rigid_Body_Revert_Surface_Power_Threshold.hpp"
#include <Mlib/Argument_List.hpp>
#include <Mlib/Components/Rigid_Body_Vehicle.hpp>
#include <Mlib/Macro_Executor/Json_Macro_Arguments.hpp>
#include <Mlib/Physics/Rigid_Body/Rigid_Body_Vehicle.hpp>
#include <Mlib/Physics/Units.hpp>
#include <Mlib/Scene/Json_User_Function_Args.hpp>
#include <Mlib/Scene_Graph/Containers/Scene.hpp>
#include <Mlib/Scene_Graph/Elements/Scene_Node.hpp>
#include <Mlib/Throw_Or_Abort.hpp>

using namespace Mlib;

namespace KnownArgs {
BEGIN_ARGUMENT_LIST;
DECLARE_ARGUMENT(node);
DECLARE_ARGUMENT(value);
}

const std::string SetRigidBodyRevertSurfacePowerThreshold::key = "set_rigid_body_revert_surface_power_threshold";

LoadSceneJsonUserFunction SetRigidBodyRevertSurfacePowerThreshold::json_user_function = [](const LoadSceneJsonUserFunctionArgs& args)
{
    args.arguments.validate(KnownArgs::options);
    SetRigidBodyRevertSurfacePowerThreshold(args.renderable_scene()).execute(args);
};

SetRigidBodyRevertSurfacePowerThreshold::SetRigidBodyRevertSurfacePowerThreshold(RenderableScene& renderable_scene) 
: LoadSceneInstanceFunction{ renderable_scene }
{}

void SetRigidBodyRevertSurfacePowerThreshold::execute(const LoadSceneJsonUserFunctionArgs& args)
{
    DanglingRef<SceneNode> node = scene.get_node(args.arguments.at<std::string>(KnownArgs::node), DP_LOC);
    auto& rb = get_rigid_body_vehicle(node);
    rb.revert_surface_power_state_.revert_surface_power_threshold_ = args.arguments.at<float>(KnownArgs::value) * meters / seconds;
}
