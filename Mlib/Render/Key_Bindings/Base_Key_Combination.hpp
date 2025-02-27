#pragma once
#include <Mlib/Render/Key_Bindings/Base_Key_Binding.hpp>
#include <list>
#include <string>

namespace Mlib {

enum class InputType;

struct BaseKeyCombination {
    std::list<BaseKeyBinding> key_bindings;
    BaseKeyBinding not_key_binding;
    std::string to_string(InputType filter) const;
};

}
