#include "Base_Gamepad_Analog_Axis_Binding.hpp"
#include <Mlib/Strings/String.hpp>
#include <Mlib/Strings/String.hpp>
#include <Mlib/Throw_Or_Abort.hpp>
#include <iomanip>
#include <sstream>

using namespace Mlib;

std::string BaseAnalogAxisBinding::to_string() const {
    std::stringstream sstr;
    sstr << "(axis: " << axis << ", scale: " << std::setprecision(2) << sign_and_scale << ')';
    return sstr.str();
}

std::string BaseAnalogAxesBinding::to_string() const {
    std::list<std::string> result;
    if (joystick.has_value()) {
        result.emplace_back("(joystick: " + joystick->to_string() + ')');
    }
    if (tap.has_value()) {
        result.emplace_back("(tap: " + tap->to_string() + ')');
    }
    if (result.size() > 1) {
        return '(' + join(" | ", result) + ')';
    }
    return result.empty() ? "" : result.front();
}

const BaseAnalogAxesBinding* BaseAnalogAxesListBinding::get_analog_axes(
    const std::string& role) const
{
    if (auto it = analog_axes.find(role); it != analog_axes.end()) {
        return &it->second;
    }
    if (auto it = analog_axes.find("default"); it != analog_axes.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string BaseAnalogAxesListBinding::to_string() const {
    auto ts = [](const auto& e){
        return '(' + e.first + ": " + e.second.to_string() + ')';
    };
    if (analog_axes.size() > 1) {
        return '(' + join(" | ", analog_axes, ts) + ')';
    }
    return analog_axes.empty() ? "" : ts(*analog_axes.begin());
}
