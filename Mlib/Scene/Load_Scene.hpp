#pragma once
#include <Mlib/Macro_Executor.hpp>
#include <Mlib/Scene_Graph/Focus.hpp>
#include <vector>

namespace Mlib {

class Players;
struct PhysicsEngineConfig;
class RenderingResources;
class SceneNodeResources;
class Scene;
class PhysicsEngine;
class AbsoluteMovableIdleBinding;
class AbsoluteMovableKeyBinding;
class RelativeMovableKeyBinding;
class CameraKeyBinding;
class GunKeyBinding;
class CameraConfig;
class RenderLogic;
class RenderLogics;
class ReadPixelsLogic;
class DirtmapLogic;
class SkyboxLogic;
struct SelectedCameras;
class SubstitutionString;
class ButtonPress;

class LoadScene {
public:
    void operator () (
        const std::string& scene_filename,
        const std::string& script_filename,
        std::string& next_scene_filename,
        RenderingResources& rendering_resources,
        SceneNodeResources& scene_node_resources,
        Players& players,
        Scene& scene,
        PhysicsEngine& physics_engine,
        ButtonPress& button_press,
        std::vector<CameraKeyBinding>& camera_key_bindings,
        std::vector<AbsoluteMovableIdleBinding>& absolute_movable_idle_bindings,
        std::vector<AbsoluteMovableKeyBinding>& absolute_movable_key_bindings,
        std::vector<RelativeMovableKeyBinding>& relative_movable_key_bindings,
        std::vector<GunKeyBinding>& gun_key_bindings,
        SelectedCameras& selected_cameras,
        const CameraConfig& camera_config,
        const PhysicsEngineConfig& physics_engine_config,
        RenderLogics& render_logics,
        RenderLogic& scene_logic,
        ReadPixelsLogic& read_pixels_logic,
        DirtmapLogic& dirtmap_logic,
        SkyboxLogic& skybox_logic,
        UiFocus& ui_focus,
        SubstitutionString& substitutions,
        size_t& num_renderings,
        std::map<std::string, size_t>& selection_ids,
        bool verbose);
private:
    MacroExecutor macro_executor_;
};

}
