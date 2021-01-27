#include <Mlib/Math/Math.hpp>
#include <Mlib/Regex.hpp>
#include <Mlib/Resource_Ptr.hpp>
#include <fenv.h>
#include <iostream>

using namespace Mlib;

struct A { int a = 3; };

void test_resource_ptr() {
    resource_ptr_target<float> v{new float(5)};
    {
        resource_ptr<float> pv = v.instantiate();
        assert_isequal(*pv, 5.f);
    }
    resource_ptr_target<A> a{new A};
    {
        resource_ptr<A> b = a.instantiate();
        assert_isequal(b->a, 3);
    }
}

void test_substitute() {
    {
        std::string str = "macro_playback create-CAR_NAME -SUFFIX:-SUFFIX IF_WITH_PHYSICS: IF_RACING:IF_RACING IF_RALLY:IF_RALLY";
        std::string replacements = "-CAR_NAME:_XZ -SUFFIX:_npc1 IF_WITH_PHYSICS: IF_RACING:# IF_RALLY:";
        assert_true(substitute(str, replacements) == "macro_playback create_XZ -SUFFIX:_npc1 IF_WITH_PHYSICS: IF_RACING:# IF_RALLY:");
    }
    {
        std::string str = "IS_SMALL";
        std::string replacements = "IS_SMALL:0";
        assert_true(substitute(str, replacements) == "0");
    }
}

#ifdef _MSC_VER
#pragma float_control(except, on)
#endif

int main(int argc, const char** argv) {
    #ifdef __linux__
    feenableexcept(FE_INVALID);
    #endif

    test_resource_ptr();
    test_substitute();
    return 0;
}
