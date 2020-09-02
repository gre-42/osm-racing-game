#include "ListView.hpp"
#include <Mlib/Render/Key_Bindings/Base_Key_Binding.hpp>
#include <Mlib/Render/Text/Renderable_Text.hpp>

namespace Mlib {

template <class TOption>
ListView<TOption>::ListView(
    GLFWwindow* window,
    const std::vector<TOption>& options,
    const std::string& ttf_filename,
    const FixedArray<float, 2>& position,
    float font_height_pixels,
    float line_distance_pixels,
    const std::function<std::string(const TOption&)>& transformation)
: renderable_text_{new RenderableText{ttf_filename, font_height_pixels}},
  options_{options},
  position_{position},
  line_distance_pixels_{line_distance_pixels},
  transformation_{transformation},
  selection_index_{0},
  window_{window}
{}

template <class TOption>
ListView<TOption>::~ListView()
{}

template <class TOption>
void ListView<TOption>::handle_input() {
    button_press_.update(window_);
    if (button_press_.key_pressed({key: "UP", joystick_axis: "2", joystick_axis_sign: -1})) {
        if (selection_index_ > 0) {
            --selection_index_;
        }
    }
    if (button_press_.key_pressed({key: "DOWN", joystick_axis: "2", joystick_axis_sign: 1})) {
        if (!options_.empty() && (selection_index_ < options_.size() - 1)) {
            ++selection_index_;
        }
    }
}

template <class TOption>
void ListView<TOption>::render(int width, int height, bool periodic_position)
{
    assert_true(window_ != nullptr);
    std::stringstream sstr;
    size_t i = 0;
    for(const auto& s : options_) {
        if (i++ == selection_index_) {
            sstr << transformation_(s) << " <-----" << std::endl;
        } else {
            sstr << transformation_(s) << std::endl;
        }
    }
    renderable_text_->render(position_, sstr.str(), width, height, line_distance_pixels_, periodic_position);
}

template <class TOption>
bool ListView<TOption>::has_selected_element() const {
    return selection_index_ < options_.size();
}

template <class TOption>
const TOption& ListView<TOption>::selected_element() const {
    assert_true(has_selected_element());
    return options_[selection_index_];
}

}
