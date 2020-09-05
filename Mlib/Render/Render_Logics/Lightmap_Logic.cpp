#include "Lightmap_Logic.hpp"
#include <Mlib/Log.hpp>
#include <Mlib/Render/CHK.hpp>
#include <Mlib/Render/Render_Config.hpp>
#include <Mlib/Render/Rendered_Scene_Descriptor.hpp>
#include <Mlib/Render/Rendering_Resources.hpp>

using namespace Mlib;

LightmapLogic::LightmapLogic(
    RenderLogic& child_logic,
    RenderingResources& rendering_resources,
    LightmapUpdateCycle update_cycle,
    size_t light_resource_id,
    const std::string& black_node_name,
    bool with_depth_texture)
: child_logic_{child_logic},
  rendering_resources_{rendering_resources},
  update_cycle_{update_cycle},
  light_resource_id_{light_resource_id},
  black_node_name_{black_node_name},
  with_depth_texture_{with_depth_texture}
{}

LightmapLogic::~LightmapLogic() {}

void LightmapLogic::render(
    int width,
    int height,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    RenderResults* render_results,
    const RenderedSceneDescriptor& frame_id)
{
    LOG_FUNCTION("LightmapLogic::render");
    if (frame_id.external_render_pass.pass == ExternalRenderPass::LIGHTMAP_TO_TEXTURE) {
        throw std::runtime_error("LightmapLogic received lightmap rendering");
    }
    if ((fb_ == nullptr) || (update_cycle_ == LightmapUpdateCycle::ALWAYS)) {
        GLsizei lightmap_width = black_node_name_.empty()
            ? render_config.scene_lightmap_width
            : render_config.black_lightmap_width;
        GLsizei lightmap_height = black_node_name_.empty()
            ? render_config.scene_lightmap_height
            : render_config.black_lightmap_height;
        CHK(glViewport(0, 0, lightmap_width, lightmap_height));
        RenderedSceneDescriptor light_rsd{external_render_pass: {ExternalRenderPass::LIGHTMAP_TO_TEXTURE, black_node_name_}, time_id: 0, light_resource_id: light_resource_id_};
        if (fb_ == nullptr) {
            fb_ = std::make_unique<FrameBuffer>();
        }
        fb_->configure({width: lightmap_width, height: lightmap_height, with_depth_texture: with_depth_texture_});
        CHK(glBindFramebuffer(GL_FRAMEBUFFER, fb_->frame_buffer));
        child_logic_.render(lightmap_width, lightmap_height, render_config, scene_graph_config, render_results, light_rsd);
        CHK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        rendering_resources_.set_texture("lightmap_color" + std::to_string(light_resource_id_), fb_->texture_color_buffer);
        rendering_resources_.set_vp("lightmap_color" + std::to_string(light_resource_id_), vp());
        if (with_depth_texture_) {
            rendering_resources_.set_texture("lightmap_depth" + std::to_string(light_resource_id_), fb_->texture_depth_buffer);
            rendering_resources_.set_vp("lightmap_depth" + std::to_string(light_resource_id_), vp());
        }
        CHK(glViewport(0, 0, width, height));
    }
}

float LightmapLogic::near_plane() const {
    return child_logic_.near_plane();
}

float LightmapLogic::far_plane() const {
    return child_logic_.far_plane();
}

const FixedArray<float, 4, 4>& LightmapLogic::vp() const {
    return child_logic_.vp();
}

bool LightmapLogic::requires_postprocessing() const {
    return child_logic_.requires_postprocessing();
}
