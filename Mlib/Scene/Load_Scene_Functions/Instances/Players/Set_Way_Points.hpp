#pragma once
#include <Mlib/Scene/Json_User_Function.hpp>
#include <Mlib/Scene/Load_Scene_Instance_Function.hpp>

namespace Mlib {

class SetWayPoints: public LoadSceneInstanceFunction {
public:
    static LoadSceneJsonUserFunction json_user_function;
    static const std::string key;
private:
    explicit SetWayPoints(RenderableScene& renderable_scene);
    void execute(const LoadSceneJsonUserFunctionArgs& args);
};

}
