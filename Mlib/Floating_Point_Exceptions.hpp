#pragma once

namespace Mlib {

void enable_floating_point_exceptions();

class TemporarilyIgnoreFloatingPointExeptions {
public:
    TemporarilyIgnoreFloatingPointExeptions();
    ~TemporarilyIgnoreFloatingPointExeptions();
private:
#ifdef WIN32
    unsigned int control_word_;
#endif
#ifdef __linux__
    int fpeflags_;
#endif
};

}
