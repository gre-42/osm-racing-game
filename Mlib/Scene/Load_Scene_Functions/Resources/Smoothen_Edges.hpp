#pragma once
#include <Mlib/Scene/Json_User_Function.hpp>
#include <string>

namespace Mlib {

class SmoothenEdges {
public:
    static LoadSceneJsonUserFunction json_user_function;
    static const std::string key;
};

}
