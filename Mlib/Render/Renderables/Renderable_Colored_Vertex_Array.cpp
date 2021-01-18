#include "Renderable_Colored_Vertex_Array.hpp"
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Geometry/Mesh/Import_Bone_Weights.hpp>
#include <Mlib/Geometry/Mesh/Triangle_Rays.hpp>
#include <Mlib/Images/Coordinates_Fixed.hpp>
#include <Mlib/Images/Revert_Axis.hpp>
#include <Mlib/Images/Vectorial_Pixels.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Render/CHK.hpp>
#include <Mlib/Render/Gen_Shader_Text.hpp>
#include <Mlib/Render/Instance_Handles/Colored_Render_Program.hpp>
#include <Mlib/Render/Renderables/Renderable_Colored_Vertex_Array/Renderable_Colored_Vertex_Array_Instance.hpp>
#include <Mlib/Render/Renderables/Renderable_Colored_Vertex_Array/Substitution_Info.hpp>
#include <Mlib/Render/Rendering_Resources.hpp>
#include <Mlib/Scene_Graph/Scene.hpp>
#include <Mlib/Scene_Graph/Scene_Node.hpp>
#include <Mlib/Stats/Mean.hpp>
#include <iostream>

static const size_t ANIMATION_NINTERPOLATED = 4;
struct ShaderBoneWeight {
    unsigned char indices[ANIMATION_NINTERPOLATED];
    float weights[ANIMATION_NINTERPOLATED];
};

using namespace Mlib;

static GenShaderText vertex_shader_text_gen{[](
    const std::vector<std::pair<TransformationMatrix<float, 3>, Light*>>& lights,
    const std::vector<size_t>& light_noshadow_indices,
    const std::vector<size_t>& light_shadow_indices,
    const std::vector<size_t>& black_shadow_indices,
    size_t nlights,
    bool has_lightmap_color,
    bool has_lightmap_depth,
    bool has_normalmap,
    bool has_dirtmap,
    bool has_diffusivity,
    bool has_specularity,
    bool has_instances,
    bool has_lookat,
    size_t nbones,
    bool reorient_normals,
    bool orthographic)
{
    assert_true(nlights == lights.size());
    std::stringstream sstr;
    sstr << "#version 330 core" << std::endl;
    sstr << "uniform mat4 MVP;" << std::endl;
    if (has_diffusivity || has_specularity) {
        sstr << "uniform mat4 M;" << std::endl;
    }
    sstr << "layout (location=0) in vec3 vPos;" << std::endl;
    sstr << "layout (location=1) in vec3 vCol;" << std::endl;
    sstr << "layout (location=2) in vec2 vTexCoord;" << std::endl;
    if (has_diffusivity || has_specularity) {
        sstr << "layout (location=3) in vec3 vNormal;" << std::endl;
    }
    if (has_normalmap) {
        sstr << "layout (location=4) in vec3 vTangent;" << std::endl;
    }
    if (has_instances) {
        sstr << "layout (location=5) in vec3 instancePosition;" << std::endl;
    }
    if (nbones != 0) {
        sstr << "layout (location=6) in lowp uvec" << ANIMATION_NINTERPOLATED << " bone_ids;" << std::endl;
        sstr << "layout (location=7) in vec" << ANIMATION_NINTERPOLATED << " bone_weights;" << std::endl;
        sstr << "uniform vec3 bone_positions[" << nbones << "];" << std::endl;
        sstr << "uniform vec4 bone_quaternions[" << nbones << "];" << std::endl;
    }
    sstr << "out vec3 color;" << std::endl;
    sstr << "out vec2 tex_coord;" << std::endl;
    if (has_lightmap_color || has_lightmap_depth) {
        if (lights.empty()) {
            throw std::runtime_error("No lights despite has_lightmap_color or has_lightmap_depth");
        }
        sstr << "uniform mat4 MVP_light[" << lights.size() << "];" << std::endl;
        // vec4 to avoid clipping problems
        sstr << "out vec4 FragPosLightSpace[" << lights.size() << "];" << std::endl;
    }
    if (has_normalmap) {
        sstr << "out vec3 tangent;" << std::endl;
        sstr << "out vec3 bitangent;" << std::endl;
    }
    if (has_dirtmap) {
        sstr << "uniform mat4 MVP_dirtmap;" << std::endl;
        sstr << "out vec2 tex_coord_dirtmap;" << std::endl;
    }
    if (reorient_normals || has_specularity) {
        sstr << "out vec3 FragPos;" << std::endl;
    }
    if (has_diffusivity || has_specularity) {
        sstr << "out vec3 Normal;" << std::endl;
    }
    if (has_lookat) {
        if (orthographic) {
            sstr << "uniform vec3 viewDir;" << std::endl;
        } else {
            sstr << "uniform vec3 viewPos;" << std::endl;
        }
    }
    sstr << "void main()" << std::endl;
    sstr << "{" << std::endl;
    sstr << "    vec3 vPosInstance;" << std::endl;
    if (nbones != 0) {
        sstr << "    vPosInstance = vec3(0, 0, 0);" << std::endl;
        for (size_t k = 0; k < ANIMATION_NINTERPOLATED; ++k) {
            static std::map<char, char> m{{0, 'x'}, {1, 'y'}, {2, 'z'}, {3, 'w'}};
            sstr << "    {" << std::endl;
            sstr << "        lowp uint i = bone_ids." << m.at(k) << ";" << std::endl;
            sstr << "        float weight = bone_weights." << m.at(k) << ";" << std::endl;
            sstr << "        vec3 o = bone_positions[i];" << std::endl;
            sstr << "        vec4 v = bone_quaternions[i];" << std::endl;
            sstr << "        vec3 p = vPos;" << std::endl;
            sstr << "        vPosInstance += weight * (o + p + 2 * cross(v.xyz, cross(v.xyz, p) + v.w * p));" << std::endl;
            sstr << "    }" << std::endl;
        }
    } else {
        sstr << "    vPosInstance = vPos;" << std::endl;
    }
    if (has_lookat && !has_instances) {
        throw std::runtime_error("has_lookat requires has_instances");
    }
    if (has_instances && has_lookat) {
        if (orthographic) {
            sstr << "    vec2 dxz = viewDir.xz;" << std::endl;
        } else {
            sstr << "    vec2 dxz = normalize(viewPos.xz - instancePosition.xz);" << std::endl;
        }
        sstr << "    vec3 dz = vec3(dxz.x, 0, dxz.y);" << std::endl;
        sstr << "    vec3 dy = vec3(0, 1, 0);" << std::endl;
        sstr << "    vec3 dx = normalize(cross(dy, dz));" << std::endl;
        sstr << "    mat3 lookat;" << std::endl;
        sstr << "    lookat[0] = dx;" << std::endl;
        sstr << "    lookat[1] = dy;" << std::endl;
        sstr << "    lookat[2] = dz;" << std::endl;
        sstr << "    vPosInstance = lookat * vPosInstance + instancePosition;" << std::endl;
    } else if (has_instances && !has_lookat) {
        sstr << "    vPosInstance = vPosInstance + instancePosition;" << std::endl;
    }
    sstr << "    gl_Position = MVP * vec4(vPosInstance, 1.0);" << std::endl;
    sstr << "    color = vCol;" << std::endl;
    sstr << "    tex_coord = vTexCoord;" << std::endl;
    if (has_lightmap_color || has_lightmap_depth) {
        sstr << "    for (int i = 0; i < " << lights.size() << "; ++i) {" << std::endl;
        sstr << "        FragPosLightSpace[i] = MVP_light[i] * vec4(vPosInstance, 1.0);" << std::endl;
        sstr << "    }" << std::endl;
    }
    if (has_dirtmap) {
        sstr << "    vec4 pos4_dirtmap = MVP_dirtmap * vec4(vPosInstance, 1.0);" << std::endl;
        sstr << "    tex_coord_dirtmap = (pos4_dirtmap.xy / pos4_dirtmap.w + 1) / 2;" << std::endl;
    }
    if (reorient_normals || has_specularity) {
        sstr << "    FragPos = vec3(M * vec4(vPosInstance, 1.0));" << std::endl;
    }
    if (has_diffusivity || has_specularity) {
        sstr << "    Normal = mat3(M) * vNormal;" << std::endl;
    }
    if (has_normalmap) {
        sstr << "    tangent = mat3(M) * vTangent;" << std::endl;
        sstr << "    bitangent = cross(Normal, tangent);" << std::endl;
    }
    sstr << "}" << std::endl;
    return sstr.str();
}};

enum class OcclusionType {
    OFF,
    OCCLUDED,
    OCCLUDER
};

static GenShaderText fragment_shader_text_textured_rgb_gen{[](
    const std::vector<std::pair<TransformationMatrix<float, 3>, Light*>>& lights,
    const std::vector<size_t>& light_noshadow_indices,
    const std::vector<size_t>& light_shadow_indices,
    const std::vector<size_t>& black_shadow_indices,
    size_t nlights,
    bool has_texture,
    bool has_lightmap_color,
    bool has_lightmap_depth,
    bool has_normalmap,
    bool has_dirtmap,
    const OrderableFixedArray<float, 3>& ambience,
    const OrderableFixedArray<float, 3>& diffusivity,
    const OrderableFixedArray<float, 3>& specularity,
    float alpha_threshold,
    OcclusionType occlusion_type,
    bool reorient_normals,
    bool orthographic,
    float dirtmap_discreteness)
{
    assert_true(nlights == lights.size());
    std::stringstream sstr;
    sstr << "#version 330 core" << std::endl;
    sstr << "in vec3 color;" << std::endl;
    if (has_texture) {
        sstr << "in vec2 tex_coord;" << std::endl;
    }
    sstr << "out vec4 frag_color;" << std::endl;
    sstr << "uniform sampler2D texture1;" << std::endl;
    if (has_lightmap_color || has_lightmap_depth) {
        if (lights.empty()) {
            throw std::runtime_error("No lights despite has_lightmap_color or has_lightmap_depth");
        }
        sstr << "in vec4 FragPosLightSpace[" << lights.size() << "];" << std::endl;
    }
    assert_true(!(has_lightmap_color && has_lightmap_depth));
    if (has_lightmap_color) {
        sstr << "uniform sampler2D texture_light_color[" << lights.size() << "];" << std::endl;
    }
    if (has_lightmap_depth) {
        sstr << "uniform sampler2D texture_light_depth[" << lights.size() << "];" << std::endl;
    }
    if (has_normalmap) {
        sstr << "in vec3 tangent;" << std::endl;
        sstr << "in vec3 bitangent;" << std::endl;
        sstr << "uniform sampler2D texture_normalmap;" << std::endl;
    }
    if (has_dirtmap) {
        sstr << "in vec2 tex_coord_dirtmap;" << std::endl;
        sstr << "uniform sampler2D texture_dirtmap;" << std::endl;
        sstr << "uniform sampler2D texture_dirt;" << std::endl;
    }
    if (!diffusivity.all_equal(0) || !specularity.all_equal(0)) {
        sstr << "in vec3 Normal;" << std::endl;

        // sstr << "uniform vec3 lightPos;" << std::endl;
        sstr << "uniform vec3 lightDir[" << lights.size() << "];" << std::endl;
    }
    if (!ambience.all_equal(0)) {
        sstr << "uniform vec3 lightAmbience[" << lights.size() << "];" << std::endl;
    }
    if (!diffusivity.all_equal(0)) {
        sstr << "uniform vec3 lightDiffusivity[" << lights.size() << "];" << std::endl;
    }
    if (!specularity.all_equal(0)) {
        sstr << "uniform vec3 lightSpecularity[" << lights.size() << "];" << std::endl;
    }
    if (reorient_normals || !specularity.all_equal(0)) {
        sstr << "in vec3 FragPos;" << std::endl;
        if (orthographic) {
            sstr << "uniform vec3 viewDir;" << std::endl;
        } else {
            sstr << "uniform vec3 viewPos;" << std::endl;
        }
    }
    if (!lights.empty()) {
        if (!ambience.all_equal(0)) {
            sstr << "vec3 phong_ambient(in int i) {" << std::endl;
            sstr << "    vec3 fragAmbience = vec3(" << ambience(0) << ", " << ambience(1) << ", " << ambience(2) << ");" << std::endl;
            sstr << "    return fragAmbience * lightAmbience[i];" << std::endl;
            sstr << "}" << std::endl;
        }
        if (!diffusivity.all_equal(0)) {
            sstr << "vec3 phong_diffuse(in int i, in vec3 norm) {" << std::endl;
            sstr << "    vec3 fragDiffusivity = vec3(" << diffusivity(0) << ", " << diffusivity(1) << ", " << diffusivity(2) << ");" << std::endl;
            sstr << "    float diff = max(dot(norm, lightDir[i]), 0.0);" << std::endl;
            sstr << "    return fragDiffusivity * diff * lightDiffusivity[i];" << std::endl;
            sstr << "}" << std::endl;
        }
        if (!specularity.all_equal(0)) {
            sstr << "vec3 phong_specular(in int i, in vec3 norm) {" << std::endl;
            sstr << "    vec3 fragSpecularity = vec3(" << specularity(0) << ", " << specularity(1) << ", " << specularity(2) << ");" << std::endl;
            if (!orthographic) {
                sstr << "    vec3 viewDir = normalize(viewPos - FragPos);" << std::endl;
            }
            sstr << "    vec3 reflectDir = reflect(-lightDir[i], norm);  " << std::endl;
            sstr << "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 4);" << std::endl;
            sstr << "    return fragSpecularity * spec * lightSpecularity[i];" << std::endl;
            sstr << "}" << std::endl;
        }
    }
    sstr << "void main()" << std::endl;
    sstr << "{" << std::endl;
    if ((alpha_threshold < 1) || has_texture) {
        sstr << "    vec4 texture1_color = texture(texture1, tex_coord);" << std::endl;
    }
    if (alpha_threshold < 1) {
        if (!has_texture) {
            throw std::runtime_error("Alpha threshold requires texture");
        }
        sstr << "    if (texture1_color.a < " << alpha_threshold << ")" << std::endl;
        sstr << "        discard;" << std::endl;
    }
    sstr << "    vec3 fragBrightness = vec3(0, 0, 0);" << std::endl;
    if (!diffusivity.all_equal(0) || !specularity.all_equal(0)) {
        // sstr << "    vec3 norm = normalize(Normal);" << std::endl;
        sstr << "    vec3 norm = normalize(Normal);" << std::endl;
        // sstr << "    vec3 lightDir = normalize(lightPos - FragPos);" << std::endl;
    }
    if (reorient_normals) {
        if (orthographic) {
            sstr << "    norm *= sign(dot(norm, viewDir));" << std::endl;
        } else {
            sstr << "    norm *= sign(dot(norm, viewPos - FragPos));" << std::endl;
        }
    }
    if (has_normalmap) {
        sstr << "    vec3 tang = normalize(tangent);" << std::endl;
        sstr << "    vec3 bitan = normalize(bitangent);" << std::endl;
        sstr << "    mat3 TBN = mat3(tang, bitan, norm);" << std::endl;
        // sstr << "    mat3 TBN = mat3(tangent, bitangent, norm);" << std::endl;
        sstr << "    vec3 tnorm = 2 * texture(texture_normalmap, tex_coord).rgb - 1;" << std::endl;
        sstr << "    norm = normalize(TBN * tnorm);" << std::endl;
    }
    if (!ambience.all_equal(0) || !diffusivity.all_equal(0) || !specularity.all_equal(0)) {
        if (!light_noshadow_indices.empty()) {
            sstr << "    int light_noshadow_indices[" << light_noshadow_indices.size() << "] = int[](" << std::endl;
            for (size_t i : light_noshadow_indices) {
                sstr << "        " << i << ((i != light_noshadow_indices.back()) ? "," : "") << std::endl;
            }
            sstr << "    );" << std::endl;
        }
        if (!has_lightmap_depth && !light_shadow_indices.empty()) {
            sstr << "    vec3 color_fac = vec3(1, 1, 1);" << std::endl;
            sstr << "    int light_shadow_indices[" << light_shadow_indices.size() << "] = int[](" << std::endl;
            for (size_t i : light_shadow_indices) {
                sstr << "        " << i << ((i != light_shadow_indices.back()) ? "," : "") << std::endl;
            }
            sstr << "    );" << std::endl;
        }
        if (has_lightmap_color && !black_shadow_indices.empty()) {
            sstr << "    int black_shadow_indices[" << black_shadow_indices.size() << "] = int[](" << std::endl;
            for (size_t i : black_shadow_indices) {
                sstr << "        " << i << ((i != black_shadow_indices.back()) ? "," : "") << std::endl;
            }
            sstr << "    );" << std::endl;
        }
        assert_true(!(has_lightmap_color && has_lightmap_depth));
        if (has_lightmap_color && !black_shadow_indices.empty()) {
            sstr << "    for (int j = 0; j < " << black_shadow_indices.size() << "; ++j) {" << std::endl;
            sstr << "        int i = black_shadow_indices[j];" << std::endl;
            sstr << "        vec3 proj_coords11 = FragPosLightSpace[i].xyz / FragPosLightSpace[i].w;" << std::endl;
            sstr << "        vec3 proj_coords01 = proj_coords11 * 0.5 + 0.5;" << std::endl;
            sstr << "        color_fac *= texture(texture_light_color[i], proj_coords01.xy).rgb;" << std::endl;
            sstr << "    }" << std::endl;
        }
        if (has_lightmap_depth) {
            sstr << "    for (int i = 0; i < " << lights.size() << "; ++i) {" << std::endl;
            sstr << "        vec3 proj_coords11 = FragPosLightSpace[i].xyz / FragPosLightSpace[i].w;" << std::endl;
            sstr << "        vec3 proj_coords01 = proj_coords11 * 0.5 + 0.5;" << std::endl;
            sstr << "        if (proj_coords01.z - 0.00002 < texture(texture_light_depth[i], proj_coords01.xy).r) {" << std::endl;
            if (!ambience.all_equal(0)) {
                sstr << "            fragBrightness += phong_ambient(i);" << std::endl;
            }
            if (!diffusivity.all_equal(0)) {
                sstr << "            fragBrightness += phong_diffuse(i, norm);" << std::endl;
            }
            if (!specularity.all_equal(0)) {
                sstr << "            fragBrightness += phong_specular(i, norm);" << std::endl;
            }
            sstr << "        }" << std::endl;
            sstr << "    }" << std::endl;
        }
        if (!has_lightmap_depth && !light_shadow_indices.empty()) {
            sstr << "    for (int j = 0; j < " << light_shadow_indices.size() << "; ++j) {" << std::endl;
            sstr << "        int i = light_shadow_indices[j];" << std::endl;
            if (has_lightmap_color) {
                sstr << "        vec3 proj_coords11 = FragPosLightSpace[i].xyz / FragPosLightSpace[i].w;" << std::endl;
                sstr << "        vec3 proj_coords01 = proj_coords11 * 0.5 + 0.5;" << std::endl;
                sstr << "        vec3 light_fac = texture(texture_light_color[i], proj_coords01.xy).rgb;" << std::endl;
            } else {
                sstr << "        vec3 light_fac = vec3(1, 1, 1);" << std::endl;
            }
            if (!ambience.all_equal(0)) {
                sstr << "        fragBrightness += light_fac * phong_ambient(i);" << std::endl;
            }
            if (!diffusivity.all_equal(0)) {
                sstr << "        fragBrightness += light_fac * phong_diffuse(i, norm);" << std::endl;
            }
            if (!specularity.all_equal(0)) {
                sstr << "        fragBrightness += light_fac * phong_specular(i, norm);" << std::endl;
            }
            sstr << "    }" << std::endl;
        }
        if (!light_noshadow_indices.empty()) {
            sstr << "    for (int j = 0; j < " << light_noshadow_indices.size() << "; ++j) {" << std::endl;
            sstr << "        int i = light_noshadow_indices[j];" << std::endl;
            if (!ambience.all_equal(0)) {
                sstr << "        fragBrightness += phong_ambient(i);" << std::endl;
            }
            if (!diffusivity.all_equal(0)) {
                sstr << "        fragBrightness += phong_diffuse(i, norm);" << std::endl;
            }
            if (!specularity.all_equal(0)) {
                sstr << "        fragBrightness += phong_specular(i, norm);" << std::endl;
            }
            sstr << "    }" << std::endl;
        }
    }
    if (has_lightmap_color && !black_shadow_indices.empty()) {
        sstr << "    fragBrightness *= color_fac;" << std::endl;
    }
    if (!has_texture && has_dirtmap) {
        std::runtime_error("Combination of (!has_texture && has_dirtmap) is not supported");
    }
    if (has_dirtmap) {
        sstr << "    vec4 dirtiness = texture(texture_dirtmap, tex_coord_dirtmap);" << std::endl;
        sstr << "    dirtiness.r = clamp(0.5 + " << dirtmap_discreteness << " * (dirtiness.r - 0.5), 0, 1);" << std::endl;
        sstr << "    dirtiness.g = clamp(0.5 + " << dirtmap_discreteness << " * (dirtiness.g - 0.5), 0, 1);" << std::endl;
        sstr << "    dirtiness.b = clamp(0.5 + " << dirtmap_discreteness << " * (dirtiness.b - 0.5), 0, 1);" << std::endl;
        // sstr << "    dirtiness.r += clamp(0.005 + 80 * (0.98 - norm.y), 0, 1);" << std::endl;
        // sstr << "    dirtiness.g += clamp(0.005 + 80 * (0.98 - norm.y), 0, 1);" << std::endl;
        // sstr << "    dirtiness.b += clamp(0.005 + 80 * (0.98 - norm.y), 0, 1);" << std::endl;
        sstr << "    frag_color = texture1_color * (1 - dirtiness)" << std::endl;
        sstr << "               + texture(texture_dirt, tex_coord) * dirtiness;" << std::endl;
        sstr << "    frag_color *= vec4(color, 1.0);" << std::endl;
    } else if (has_texture) {
        sstr << "    frag_color = texture1_color * vec4(color, 1.0);" << std::endl;
    } else {
        sstr << "    frag_color = vec4(color, 1.0);" << std::endl;
    }
    sstr << "    frag_color.rgb *= fragBrightness;" << std::endl;
    if (occlusion_type == OcclusionType::OCCLUDED) {
        sstr << "    frag_color.r = 1;" << std::endl;
        sstr << "    frag_color.g = 1;" << std::endl;
        sstr << "    frag_color.b = 1;" << std::endl;
    }
    if (occlusion_type == OcclusionType::OCCLUDER) {
        sstr << "    frag_color.r = 0.5;" << std::endl;
        sstr << "    frag_color.g = 0.5;" << std::endl;
        sstr << "    frag_color.b = 0.5;" << std::endl;
    }
    sstr << "}" << std::endl;
    return sstr.str();
}};

RenderableColoredVertexArray::RenderableColoredVertexArray(
    const std::shared_ptr<AnimatedColoredVertexArrays>& triangles,
    std::map<const ColoredVertexArray*, std::vector<TransformationMatrix<float, 3>>>* instances,
    RenderingResources& rendering_resources)
: triangles_res_{triangles},
  rendering_resources_{rendering_resources},
  instances_{instances},
  textures_preloaded_{false}
{
#ifdef DEBUG
    triangles_res_->check_consistency();
#endif
}

RenderableColoredVertexArray::RenderableColoredVertexArray(
    const std::list<std::shared_ptr<ColoredVertexArray>>& triangles,
    std::map<const ColoredVertexArray*, std::vector<TransformationMatrix<float, 3>>>* instances,
    RenderingResources& rendering_resources)
: RenderableColoredVertexArray{
    std::make_shared<AnimatedColoredVertexArrays>(),
    instances,
    rendering_resources}
{
    triangles_res_->cvas = triangles;
#ifdef DEBUG
    triangles_res_->check_consistency();
#endif
}

RenderableColoredVertexArray::RenderableColoredVertexArray(
    const std::shared_ptr<ColoredVertexArray>& triangles,
    std::map<const ColoredVertexArray*, std::vector<TransformationMatrix<float, 3>>>* instances,
    RenderingResources& rendering_resources)
: RenderableColoredVertexArray(
    std::list<std::shared_ptr<ColoredVertexArray>>{triangles},
    instances,
    rendering_resources)
{
    triangles_res_->cvas.push_back(triangles);
#ifdef DEBUG
    triangles_res_->check_consistency();
#endif
}

RenderableColoredVertexArray::~RenderableColoredVertexArray()
{}

void RenderableColoredVertexArray::instantiate_renderable(const std::string& name, SceneNode& scene_node, const SceneNodeResourceFilter& resource_filter) const
{
#ifdef DEBUG
    triangles_res_->check_consistency();
#endif
    if (!textures_preloaded_ && (glfwGetCurrentContext() != nullptr)) {
        for (auto& cva : triangles_res_->cvas) {
            rendering_resources_.preload(cva->material.texture_descriptor);
        }
        textures_preloaded_ = true;
    }
    scene_node.add_renderable(name, std::make_shared<RenderableColoredVertexArrayInstance>(
        shared_from_this(),
        resource_filter));
}

std::shared_ptr<AnimatedColoredVertexArrays> RenderableColoredVertexArray::get_animated_arrays() const {
    return triangles_res_;
}

void RenderableColoredVertexArray::generate_triangle_rays(size_t npoints, const FixedArray<float, 3>& lengths, bool delete_triangles) {
    for (auto& t : triangles_res_->cvas) {
        auto r = Mlib::generate_triangle_rays(t->triangles, npoints, lengths);
        t->lines.reserve(t->lines.size() + r.size());
        for (const auto& l : r) {
            t->lines.push_back({
                ColoredVertex{
                    position: l(0),
                    color: {1, 1, 1},
                    uv: {0, 0}
                },
                ColoredVertex{
                    position: l(1),
                    color: {1, 1, 1},
                    uv: {0, 1}
                }
            });
        }
        if (delete_triangles) {
            t->triangles.clear();
        }
    }
}

void RenderableColoredVertexArray::generate_ray(const FixedArray<float, 3>& from, const FixedArray<float, 3>& to) {
    if (triangles_res_->cvas.size() != 1) {
        throw std::runtime_error("generate_ray requires exactly one triangle mesh");
    }
    triangles_res_->cvas.front()->lines.push_back({
        ColoredVertex{
            position: from,
            color: {1, 1, 1},
            uv: {0, 0}
        },
        ColoredVertex{
            position: to,
            color: {1, 1, 1},
            uv: {0, 1}
        }
    });
}

void RenderableColoredVertexArray::downsample(size_t factor) {
    for (auto& t : triangles_res_->cvas) {
        t->downsample_triangles(factor);
    }
}

AggregateMode RenderableColoredVertexArray::aggregate_mode() const {
    std::set<AggregateMode> aggregate_modes;
    if (triangles_res_->cvas.empty()) {
        throw std::runtime_error("Cannot determine aggregate mode of empty array");
    }
    for (const auto& t : triangles_res_->cvas) {
        aggregate_modes.insert(t->material.aggregate_mode);
    }
    if (aggregate_modes.size() != 1) {
        throw std::runtime_error("aggregate_mode is not unique");
    }
    return triangles_res_->cvas.front()->material.aggregate_mode;
}

const ColoredRenderProgram& RenderableColoredVertexArray::get_render_program(
    const RenderProgramIdentifier& id,
    const std::vector<std::pair<TransformationMatrix<float, 3>, Light*>>& filtered_lights,
    const std::vector<size_t>& light_noshadow_indices,
    const std::vector<size_t>& light_shadow_indices,
    const std::vector<size_t>& black_shadow_indices) const
{
    auto& rps = rendering_resources_.render_programs();
    if (auto it = rps.find(id); it != rps.end()) {
        return *it->second;
    }
    std::lock_guard guard{mutex_};
    if (auto it = rps.find(id); it != rps.end()) {
        return *it->second;
    }
    auto rp = std::make_unique<ColoredRenderProgram>();
    OcclusionType occlusion_type;
    if (id.calculate_lightmap) {
        if (id.occluder_type == OccluderType::WHITE) {
            occlusion_type = OcclusionType::OCCLUDED;
        } else if (id.occluder_type == OccluderType::BLACK) {
            occlusion_type = OcclusionType::OCCLUDER;
        } else {
            throw std::runtime_error("get_render_program: calculate_lightmap requires occlusion");
        }
    } else {
        occlusion_type = OcclusionType::OFF;
    }
    assert_true(triangles_res_->bone_indices.empty() == !triangles_res_->skeleton);
    rp->generate(
        vertex_shader_text_gen(
            filtered_lights,
            light_noshadow_indices,
            light_shadow_indices,
            black_shadow_indices,
            filtered_lights.size(),
            id.has_lightmap_color,
            id.has_lightmap_depth,
            id.has_normalmap,
            id.has_dirtmap,
            !id.diffusivity.all_equal(0),
            !id.specularity.all_equal(0),
            id.has_instances,
            id.has_lookat,
            triangles_res_->bone_indices.size(),
            id.reorient_normals,
            id.orthographic),
        fragment_shader_text_textured_rgb_gen(
            filtered_lights,
            light_noshadow_indices,
            light_shadow_indices,
            black_shadow_indices,
            filtered_lights.size(),
            id.has_texture,
            id.has_lightmap_color,
            id.has_lightmap_depth,
            id.has_normalmap,
            id.has_dirtmap,
            id.ambience,
            id.diffusivity,
            id.specularity,
            (id.blend_mode == BlendMode::BINARY) || (id.blend_mode == BlendMode::BINARY_ADD)
                ? (id.calculate_lightmap ? 0.1 : 0.5)
                : 1,
            occlusion_type,
            id.reorient_normals,
            id.orthographic,
            id.dirtmap_discreteness));

    rp->mvp_location = checked_glGetUniformLocation(rp->program, "MVP");
    if (id.has_texture) {
        rp->texture1_location = checked_glGetUniformLocation(rp->program, "texture1");
    } else {
        rp->texture1_location = 0;
    }
    if (id.has_lightmap_color || id.has_lightmap_depth) {
        for (size_t i = 0; i < filtered_lights.size(); ++i) {
            rp->mvp_light_locations[i] = checked_glGetUniformLocation(rp->program, ("MVP_light[" + std::to_string(i) + "]").c_str());
        }
    } else {
        // Do nothing
        // rp->mvp_light_location = 0;
    }
    assert(!(id.has_lightmap_color && id.has_lightmap_depth));
    if (id.has_lightmap_color) {
        size_t i = 0;
        for (const auto& l : filtered_lights) {
            if (l.second->shadow) {
                rp->texture_lightmap_color_locations[i] = checked_glGetUniformLocation(rp->program, ("texture_light_color[" + std::to_string(i) + "]").c_str());
            }
            ++i;
        }
    } else {
        // Do nothing
        // rp->texture_lightmap_color_location = 0;
    }
    if (id.has_lightmap_depth) {
        for (size_t i = 0; i < filtered_lights.size(); ++i) {
            rp->texture_lightmap_depth_locations[i] = checked_glGetUniformLocation(rp->program, ("texture_light_depth[" + std::to_string(i) + "]").c_str());
        }
    } else {
        // Do nothing
        // rp->texture_lightmap_depth_location = 0;
    }
    if (id.has_normalmap) {
        rp->texture_normalmap_location = checked_glGetUniformLocation(rp->program, "texture_normalmap");
    } else {
        rp->texture_normalmap_location = 0;
    }
    if (id.has_dirtmap) {
        rp->mvp_dirtmap_location = checked_glGetUniformLocation(rp->program, "MVP_dirtmap");
        rp->texture_dirtmap_location = checked_glGetUniformLocation(rp->program, "texture_dirtmap");
        rp->texture_dirt_location = checked_glGetUniformLocation(rp->program, "texture_dirt");
    } else {
        rp->mvp_dirtmap_location = 0;
        rp->texture_dirtmap_location = 0;
        rp->texture_dirt_location = 0;
    }
    if (!id.diffusivity.all_equal(0) || !id.specularity.all_equal(0)) {
        rp->m_location = checked_glGetUniformLocation(rp->program, "M");
        for (size_t i = 0; i < filtered_lights.size(); ++i) {
            rp->light_dir_locations[i] = checked_glGetUniformLocation(rp->program, ("lightDir[" + std::to_string(i) + "]").c_str());
        }
    } else {
        rp->m_location = 0;
    }
    // rp->light_position_location = checked_glGetUniformLocation(rp->program, "lightPos");
    assert_true(triangles_res_->bone_indices.empty() == !triangles_res_->skeleton);
    for (size_t i = 0; i < triangles_res_->bone_indices.size(); ++i) {
        rp->pose_positions[i] = checked_glGetUniformLocation(rp->program, ("bone_positions[" + std::to_string(i) + "]").c_str());
        rp->pose_quaternions[i] = checked_glGetUniformLocation(rp->program, ("bone_quaternions[" + std::to_string(i) + "]").c_str());
    }
    for (size_t i = 0; i < filtered_lights.size(); ++i) {
        if (!id.ambience.all_equal(0)) {
            rp->light_ambiences[i] = checked_glGetUniformLocation(rp->program, ("lightAmbience[" + std::to_string(i) + "]").c_str());
        }
        if (!id.diffusivity.all_equal(0)) {
            rp->light_diffusivities[i] = checked_glGetUniformLocation(rp->program, ("lightDiffusivity[" + std::to_string(i) + "]").c_str());
        }
        if (!id.specularity.all_equal(0)) {
            rp->light_specularities[i] = checked_glGetUniformLocation(rp->program, ("lightSpecularity[" + std::to_string(i) + "]").c_str());
        }
    }
    if (id.has_lookat || !id.specularity.all_equal(0) || id.reorient_normals) {
        if (id.orthographic) {
            rp->view_dir = checked_glGetUniformLocation(rp->program, "viewDir");
            rp->view_pos = 0;
        } else {
            rp->view_dir = 0;
            rp->view_pos = checked_glGetUniformLocation(rp->program, "viewPos");
        }
    } else {
        rp->view_dir = 0;
        rp->view_pos = 0;
    }

    auto& result = *rp;
    rps.insert(std::make_pair(id, std::move(rp)));
    return result;
}

const SubstitutionInfo& RenderableColoredVertexArray::get_vertex_array(const std::shared_ptr<ColoredVertexArray>& cva) const
{
    if (cva->material.aggregate_mode != AggregateMode::OFF && instances_ == nullptr) {
        throw std::runtime_error("get_vertex_array called on aggregated object \"" + cva->name + '"');
    }
    if (auto it = vertex_arrays_.find(cva.get()); it != vertex_arrays_.end()) {
        return *it->second;
    }
    if (cva->triangles.empty()) {
        throw std::runtime_error("RenderableColoredVertexArray::get_vertex_array on empty array \"" + cva->name + '"');
    }
    std::lock_guard guard{mutex_};
    auto si = std::make_unique<SubstitutionInfo>();
    auto& va = si->va;
    // https://stackoverflow.com/a/13405205/2292832
    CHK(glGenVertexArrays(1, &va.vertex_array));
    CHK(glBindVertexArray(va.vertex_array));

    CHK(glGenBuffers(1, &va.vertex_buffer));
    CHK(glBindBuffer(GL_ARRAY_BUFFER, va.vertex_buffer));
    CHK(glBufferData(GL_ARRAY_BUFFER, sizeof(cva->triangles[0]) * cva->triangles.size(), cva->triangles.front().flat_begin(), GL_STATIC_DRAW));

    ColoredVertex* cv = nullptr;
    CHK(glEnableVertexAttribArray(0));
    CHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), &cv->position));
    CHK(glEnableVertexAttribArray(1));
    CHK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), &cv->color));
    CHK(glEnableVertexAttribArray(2));
    CHK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), &cv->uv));
    // The vertex array is cached by cva => Use material properties, not the RenderProgramIdentifier.
    if (!cva->material.diffusivity.all_equal(0) || !cva->material.specularity.all_equal(0)) {
        CHK(glEnableVertexAttribArray(3));
        CHK(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), &cv->normal));
    }
    // The vertex array is cached by cva => Use material properties, not the RenderProgramIdentifier.
    if (!cva->material.texture_descriptor.normal.empty()) {
        CHK(glEnableVertexAttribArray(4));
        CHK(glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), &cv->tangent));
    }
    if (instances_ != nullptr) {
        const std::vector<TransformationMatrix<float, 3>>& inst = instances_->at(cva.get());
        if (inst.empty()) {
            throw std::runtime_error("RenderableColoredVertexArray::get_vertex_array received empty instances \"" + cva->name + '"');
        }
        std::vector<FixedArray<float, 3>> positions;
        positions.reserve(inst.size());
        for (const TransformationMatrix<float, 3>& m : inst) {
            positions.push_back(m.t());
        }
        CHK(glGenBuffers(1, &va.position_buffer));
        CHK(glBindBuffer(GL_ARRAY_BUFFER, va.position_buffer));
        CHK(glBufferData(GL_ARRAY_BUFFER, sizeof(positions[0]) * positions.size(), &positions.front(), GL_STATIC_DRAW));

        CHK(glEnableVertexAttribArray(5));
        CHK(glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(positions[0]), nullptr));
        CHK(glVertexAttribDivisor(5, 1));
    }
    assert_true(cva->triangle_bone_weights.empty() == !triangles_res_->skeleton);
    if (triangles_res_->skeleton != nullptr) {
        std::vector<FixedArray<ShaderBoneWeight, 3>> triangle_bone_weights(cva->triangle_bone_weights.size());
        for (size_t tid = 0; tid < triangle_bone_weights.size(); ++tid) {
            const auto& td = cva->triangle_bone_weights[tid];
            auto& ts = triangle_bone_weights[tid];
            for (size_t vid = 0; vid < td.length(); ++vid) {
                auto vd = td(vid);
                auto& vs = ts(vid);
                // Sort in descending order
                std::sort(vd.begin(), vd.end(), [](const BoneWeight& w0, const BoneWeight& w1){return w0.weight > w1.weight;});
                float sum_weights = 0;
                for (size_t i = 0; i < ANIMATION_NINTERPOLATED; ++i) {
                    if (i < vd.size()) {
                        if (vd[i].bone_index >= triangles_res_->bone_indices.size()) {
                            throw std::runtime_error(
                                "Bone index too large in get_vertex_array: " +
                                std::to_string(vd[i].bone_index) + " >= " +
                                std::to_string(triangles_res_->bone_indices.size()));
                        }
                        vs.indices[i] = vd[i].bone_index;
                        vs.weights[i] = vd[i].weight;
                        sum_weights += vs.weights[i];
                    } else {
                        vs.indices[i] = 0;
                        vs.weights[i] = 0;
                    }
                }
                if (sum_weights < 1e-3) {
                    throw std::runtime_error("Sum of weights too small");
                }
                if (sum_weights > 1.1) {
                    throw std::runtime_error("Sum of weights too large");
                }
                for (size_t i = 0; i < ANIMATION_NINTERPOLATED; ++i) {
                    vs.weights[i] /= sum_weights;
                }
            }
        }
        CHK(glGenBuffers(1, &va.bone_weight_buffer));
        CHK(glBindBuffer(GL_ARRAY_BUFFER, va.bone_weight_buffer));
        CHK(glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_bone_weights[0]) * triangle_bone_weights.size(), &triangle_bone_weights.front(), GL_STATIC_DRAW));

        ShaderBoneWeight* bw = nullptr;
        CHK(glEnableVertexAttribArray(6));
        CHK(glVertexAttribIPointer(6, ANIMATION_NINTERPOLATED, GL_UNSIGNED_BYTE, sizeof(ShaderBoneWeight), &bw->indices));
        CHK(glEnableVertexAttribArray(7));
        CHK(glVertexAttribPointer(7, ANIMATION_NINTERPOLATED, GL_FLOAT, GL_FALSE, sizeof(ShaderBoneWeight), &bw->weights));
    }

    CHK(glBindVertexArray(0));
    si->cva = cva;
    si->ntriangles = cva->triangles.size();
    si->nlines = cva->lines.size();
    auto& result = *si;  // store data before std::move (unique_ptr)
    vertex_arrays_.insert(std::make_pair(cva.get(), std::move(si)));
    return result;
}

void RenderableColoredVertexArray::set_absolute_joint_poses(
    const std::vector<OffsetAndQuaternion<float>>& poses)
{
    for (auto& t : triangles_res_->cvas) {
        t = t->transformed(poses);
    }
}

void RenderableColoredVertexArray::import_bone_weights(
    const AnimatedColoredVertexArrays& other_acvas,
    float max_distance)
{
    Mlib::import_bone_weights(
        *triangles_res_,
        other_acvas,
        max_distance);
}
