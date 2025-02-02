#include "Light.hpp"
#include <Mlib/Array/Fixed_Array_Hash.hpp>
#include <Mlib/Geometry/Material/Render_Pass.hpp>

using namespace Mlib;

bool Light::emits_colors() const {
    return (shadow_render_pass == ExternalRenderPassType::NONE) ||
        any(shadow_render_pass & ExternalRenderPassType::LIGHTMAP_EMITS_COLORS_MASK);
}

size_t Light::shading_hash() const {
    return hash_combine(
        ambient,
        diffuse,
        specular,
        fresnel_ambient,
        fog_ambient);
}
