#include "Flying_Camera_Logic.hpp"
#include <Mlib/Log.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Render/CHK.hpp>
#include <Mlib/Render/Key_Bindings/Base_Key_Binding.hpp>
#include <Mlib/Render/Render_Config.hpp>
#include <Mlib/Render/Rendered_Scene_Descriptor.hpp>
#include <Mlib/Render/Selected_Cameras.hpp>
#include <Mlib/Render/Ui/Button_States.hpp>
#include <Mlib/Scene_Graph/Scene.hpp>
#include <Mlib/Scene_Graph/Scene_Graph_Config.hpp>
#include <Mlib/Set_Fps.hpp>

using namespace Mlib;

static void flying_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    FlyingCameraUserClass* user_object = (FlyingCameraUserClass*)glfwGetWindowUserPointer(window);
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (action == GLFW_REPEAT || action == GLFW_PRESS) {
        if (mods & GLFW_MOD_CONTROL) {
            switch(key) {
                case GLFW_KEY_LEFT:
                    user_object->obj_angles(1) += 0.01;
                    break;
                case GLFW_KEY_RIGHT:
                    user_object->obj_angles(1) -= 0.01;
                    break;
                case GLFW_KEY_PAGE_UP:
                    user_object->obj_angles(0) += 0.01;
                    break;
                case GLFW_KEY_PAGE_DOWN:
                    user_object->obj_angles(0) -= 0.01;
                    break;
                case GLFW_KEY_KP_ADD:
                    user_object->obj_position(1) += 0.04;
                    break;
                case GLFW_KEY_KP_SUBTRACT:
                    user_object->obj_position(1) -= 0.04;
                    break;
            }
        } else {
            switch(key) {
                case GLFW_KEY_UP:
                    user_object->position(2) -= 0.04;
                    break;
                case GLFW_KEY_DOWN:
                    user_object->position(2) += 0.04;
                    break;
                case GLFW_KEY_LEFT:
                    user_object->angles(1) += 0.01;
                    break;
                case GLFW_KEY_RIGHT:
                    user_object->angles(1) -= 0.01;
                    break;
                case GLFW_KEY_PAGE_UP:
                    user_object->angles(0) += 0.01;
                    break;
                case GLFW_KEY_PAGE_DOWN:
                    user_object->angles(0) -= 0.01;
                    break;
                case GLFW_KEY_KP_ADD:
                    user_object->position(1) += 0.04;
                    break;
                case GLFW_KEY_KP_SUBTRACT:
                    user_object->position(1) -= 0.04;
                    break;
            }
        }
    }
    fullscreen_callback(window, key, scancode, action, mods);
    user_object->button_states.notify_key_event(key, action);
}

static void nofly_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    FlyingCameraUserClass* user_object = (FlyingCameraUserClass*)glfwGetWindowUserPointer(window);
    // if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    //     glfwSetWindowShouldClose(window, GLFW_TRUE);
    // }
    if (action == GLFW_REPEAT || action == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_L:
            {
                auto& cams = user_object->cameras.camera_cycle_far;
                if (cams.empty()) {
                    throw std::runtime_error("Far camera cycle is empty");
                }
                auto it = std::find(cams.begin(), cams.end(), user_object->cameras.camera_node_name());
                if (it == cams.end() || ++it == cams.end()) {
                    it = cams.begin();
                }
                user_object->cameras.set_camera_node_name(*it);
                break;
            }
            case GLFW_KEY_P:
                if (user_object->physics_set_fps != nullptr) {
                    user_object->physics_set_fps->toggle_pause_resume();
                }
                break;
        }
    }
    fullscreen_callback(window, key, scancode, action, mods);
    user_object->button_states.notify_key_event(key, action);
}

FlyingCameraLogic::FlyingCameraLogic(
    GLFWwindow* window,
    const ButtonStates& button_states,
    const Scene& scene,
    FlyingCameraUserClass& user_object,
    bool fly,
    bool rotate)
: scene_{scene},
  user_object_{user_object},
  window_{window},
  button_press_{button_states},
  fly_{fly},
  rotate_{rotate}
{
    // glfwGetWindowPos(window, &user_object_.window_x, &user_object_.window_y);
    // glfwGetWindowSize(window, &user_object_.window_width, &user_object_.window_height);
    glfwSetWindowUserPointer(window, &user_object_);
    if (fly_ || rotate_) {
        glfwSetKeyCallback(window, flying_key_callback);

        if (fly_) {
            auto cn = scene_.get_node(user_object_.cameras.camera_node_name());
            user_object_.position = cn->position();
            user_object_.angles = cn->rotation();
        }
        if (rotate_) {
            auto on = scene_.get_node("obj");
            user_object_.obj_position = on->position();
            user_object_.obj_angles = on->rotation();
        }
    } else {
        glfwSetKeyCallback(window, nofly_key_callback);
    }
}

void FlyingCameraLogic::render(
    int width,
    int height,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    RenderResults* render_results,
    const RenderedSceneDescriptor& frame_id)
{
    user_object_.button_states.update_gamepad_state();
    if (button_press_.key_pressed({key: "ESCAPE", gamepad_button: "START"})) {
        if (!user_object_.focus.empty()) {
            if (user_object_.focus.back() == Focus::MENU) {
                user_object_.focus.pop_back();
            } else if ((user_object_.focus.back() == Focus::LOADING) || (user_object_.focus.back() == Focus::COUNTDOWN) || (user_object_.focus.back() == Focus::SCENE)) {
                user_object_.focus.push_back(Focus::MENU);
            } else {
                throw std::runtime_error("Unknown focus value");
            }
        }
    }

    LOG_FUNCTION("FlyingCameraLogic::render");
    SceneNode* cn = scene_.get_node(user_object_.cameras.camera_node_name());
    if (fly_) {
        cn->set_position(user_object_.position);
        cn->set_rotation(user_object_.angles);
    }
    if (rotate_) {
        SceneNode* on = scene_.get_node(user_object_.obj_node_name);
        on->set_position(user_object_.obj_position);
        on->set_rotation(user_object_.obj_angles);
    }
}

float FlyingCameraLogic::near_plane() const {
    throw std::runtime_error("FlyingCameraLogic::near_plane not implemented");
}

float FlyingCameraLogic::far_plane() const {
    throw std::runtime_error("FlyingCameraLogic::far_plane not implemented");
}

const FixedArray<float, 4, 4>& FlyingCameraLogic::vp() const {
    throw std::runtime_error("FlyingCameraLogic::vp not implemented");
}

const TransformationMatrix<float>& FlyingCameraLogic::iv() const {
    throw std::runtime_error("FlyingCameraLogic::iv not implemented");
}

bool FlyingCameraLogic::requires_postprocessing() const {
    throw std::runtime_error("FlyingCameraLogic::requires_postprocessing not implemented");
}
