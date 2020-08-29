#pragma once
#include <map>
#include <string>

namespace Mlib {

static const std::map<std::string, int> glfw_joystick_axes {
    {"1", 0},
    {"2", 1},
    {"3", 2},
    {"4", 3},
    {"5", 4},
    {"6", 5},
    {"7", 6},
    {"8", 7},
    {"9", 8},
    {"10", 9},
    {"11", 10},
    {"12", 11},
    {"13", 12},
    {"14", 13},
    {"15", 14},
    {"16", 15},
    {"LAST", GLFW_JOYSTICK_16}};

}
