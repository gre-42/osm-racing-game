#include "Menu_Logic.hpp"
#include <Mlib/Log.hpp>
#include <Mlib/Render/Key_Bindings/Base_Key_Combination.hpp>
#include <Mlib/Render/Key_Bindings/Key_Configuration.hpp>
#include <Mlib/Render/Key_Bindings/Key_Configurations.hpp>
#include <Mlib/Render/Rendered_Scene_Descriptor.hpp>
#include <Mlib/Render/Ui/Button_States.hpp>
#include <Mlib/Scene_Graph/Focus.hpp>
#include <Mlib/Throw_Or_Abort.hpp>

namespace Mlib {
class MenuLogicKeys {
public:
    explicit MenuLogicKeys(ButtonStates& button_states)
        : start{ button_states, key_configurations, "escape", "" }
    {
        key_configurations.insert("escape", { {{{.key = "ESCAPE", .gamepad_button = "START", .tap_button = "ESCAPE"}}} });
    }
    ButtonPress start;
private:
    KeyConfigurations key_configurations;
};
}

using namespace Mlib;

MenuLogic::MenuLogic(
    MenuUserClass &user_object)
    : user_object_{user_object}
    , keys_{ std::make_unique<MenuLogicKeys>(user_object.button_states) }
{}

MenuLogic::~MenuLogic() = default;

void MenuLogic::handle_events() {
    LOG_FUNCTION("FlyingCameraLogic::render");
    if (keys_->start.keys_pressed()) {
        std::scoped_lock lock{user_object_.focuses.mutex};
        Focus focus = user_object_.focuses.focus();
        if (focus == Focus::MENU) {
            if (user_object_.focuses.size() > 1) {
                user_object_.focuses.pop_back();
            }
        } else if (user_object_.focuses.countdown_active() || any(focus & (Focus::LOADING | Focus::SCENE | Focus::GAME_OVER))) {
            user_object_.focuses.push_back(Focus::MENU);
        } else if (user_object_.focuses.game_over_countdown_active()) {
            // Do nothing, menu will show automatically after the countdown is finished
        } else if (focus != Focus::BASE) {
            THROW_OR_ABORT("Unknown focus value: " + std::to_string((int)focus));
        }
    }
}
