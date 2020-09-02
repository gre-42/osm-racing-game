#include "Scene_Selector_Logic.hpp"
#include <Mlib/Render/Key_Bindings/Base_Key_Binding.hpp>
#include <Mlib/Render/Text/Renderable_Text.hpp>
#include <Mlib/Render/Ui/Button_Press.hpp>
#include <filesystem>

namespace fs = std::filesystem;

using namespace Mlib;

SceneSelectorLogic::SceneSelectorLogic(
    const std::vector<SceneEntry>& scene_files,
    const std::string& ttf_filename,
    const FixedArray<float, 2>& position,
    float font_height_pixels,
    float line_distance_pixels,
    UiFocus& ui_focus,
    size_t submenu_id,
    std::string& scene_filename,
    bool& leave_render_loop,
    const ButtonStates& button_states)
: scene_selector_list_view_{
    button_states,
    scene_files,
    ttf_filename,
    position,
    font_height_pixels,
    line_distance_pixels,
    [](const SceneEntry& s){return s.name;}},
  ui_focus_{ui_focus},
  submenu_id_{submenu_id},
  button_press_{button_states},
  scene_filename_{scene_filename},
  leave_render_loop_{leave_render_loop}
{
    // Initialize the reference
    scene_filename_ = scene_selector_list_view_.selected_element().filename;
}

SceneSelectorLogic::~SceneSelectorLogic()
{}

void SceneSelectorLogic::render(
    int width,
    int height,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    RenderResults* render_results,
    const RenderedSceneDescriptor& frame_id)
{
    if (ui_focus_.focus == Focus::MENU) {
        if (ui_focus_.submenu_id == submenu_id_) {
            scene_selector_list_view_.handle_input();
            if (button_press_.key_pressed({key: "LEFT", joystick_axis: "1", joystick_axis_sign: -1})) {
                ui_focus_.goto_prev_submenu();
            }
            if (button_press_.key_pressed({key: "RIGHT", joystick_axis: "1", joystick_axis_sign: 1})) {
                ui_focus_.goto_next_submenu();
            }
            if (scene_selector_list_view_.has_selected_element()) {
                scene_filename_ = scene_selector_list_view_.selected_element().filename;
            }
            if (button_press_.key_pressed({key: "ENTER", gamepad_button: "A"})) {
                ui_focus_.focus = Focus::LOADING;
                leave_render_loop_ = true;
            }
        }
    }
    if (ui_focus_.focus == Focus::MENU) {
        scene_selector_list_view_.render(width, height, true); // true=periodic_position
    }
}

float SceneSelectorLogic::near_plane() const {
    throw std::runtime_error("SceneSelectorLogic::requires_postprocessing not implemented");
}

float SceneSelectorLogic::far_plane() const {
    throw std::runtime_error("SceneSelectorLogic::requires_postprocessing not implemented");
}

const FixedArray<float, 4, 4>& SceneSelectorLogic::vp() const {
    throw std::runtime_error("SceneSelectorLogic::vp not implemented");
}

bool SceneSelectorLogic::requires_postprocessing() const {
    throw std::runtime_error("SceneSelectorLogic::requires_postprocessing not implemented");
}
