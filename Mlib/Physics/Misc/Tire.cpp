#include "Tire.hpp"

using namespace Mlib;

Tire::Tire(
    const std::string& engine,
    float break_force,
    const ShockAbsorber& shock_absorber,
    const StickyWheel& sticky_wheel,
    float angle)
: shock_absorber{shock_absorber},
  sticky_wheel{sticky_wheel},
  angle{angle},
  engine{engine},
  break_force{break_force}
{}

void Tire::advance_time(float dt) {
    shock_absorber.advance_time(dt);
}
