#include "Render_To_Texture_Logic.hpp"
#include <Mlib/Log.hpp>
#include <Mlib/Render/Aggregate_Array_Renderer.hpp>
#include <Mlib/Render/Array_Instances_Renderer.hpp>
#include <Mlib/Render/CHK.hpp>
#include <Mlib/Render/Instance_Handles/Frame_Buffer.hpp>
#include <Mlib/Render/Render_Config.hpp>
#include <Mlib/Render/Render_Logics/Resource_Update_Cycle.hpp>
#include <Mlib/Render/Rendered_Scene_Descriptor.hpp>
#include <Mlib/Render/Rendering_Resources.hpp>

using namespace Mlib;

RenderToTextureLogic::RenderToTextureLogic(
    RenderLogic& child_logic,
    ResourceUpdateCycle update_cycle,
    bool with_depth_texture,
    const std::string& color_texture_name,
    const std::string& depth_texture_name,
    int texture_width,
    int texture_height)
: child_logic_{child_logic},
  rendering_context_{RenderingContextStack::rendering_context()},
  update_cycle_{update_cycle},
  with_depth_texture_{with_depth_texture},
  color_texture_name_{color_texture_name},
  depth_texture_name_{depth_texture_name},
  texture_width_{texture_width},
  texture_height_{texture_height}
{}

RenderToTextureLogic::~RenderToTextureLogic() {
    if (fb_ != nullptr) {
        rendering_context_.rendering_resources->delete_texture(color_texture_name_);
        if (with_depth_texture_) {
            rendering_context_.rendering_resources->delete_texture(depth_texture_name_);
        }
    }
}

void RenderToTextureLogic::render(
    int width,
    int height,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    RenderResults* render_results,
    const RenderedSceneDescriptor& frame_id)
{
    LOG_FUNCTION("RenderToTextureLogic::render");
    if ((fb_ == nullptr) || (update_cycle_ == ResourceUpdateCycle::ALWAYS)) {
        CHK(glViewport(0, 0, texture_width_, texture_height_));
        if (fb_ == nullptr) {
            fb_ = std::make_unique<FrameBuffer>();
        }
        fb_->configure({width: texture_width_, height: texture_height_, with_depth_texture: with_depth_texture_});
        CHK(glBindFramebuffer(GL_FRAMEBUFFER, fb_->frame_buffer));
        {
            RenderingContextGuard rrg{rendering_context_};
            child_logic_.render(texture_width_, texture_height_, render_config, scene_graph_config, render_results, frame_id);
        }

        // VectorialPixels<float, 3> vpx{ArrayShape{size_t(lightmap_width), size_t(lightmap_height)}};
        // CHK(glReadPixels(0, 0, lightmap_width, lightmap_height, GL_RGB, GL_FLOAT, vpx->flat_iterable().begin()));
        // PpmImage::from_float_rgb(vpx.to_array()).save_to_file("/tmp/lightmap.ppm");

        CHK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        rendering_context_.rendering_resources->set_texture(color_texture_name_, fb_->texture_color_buffer);
        if (with_depth_texture_) {
            rendering_context_.rendering_resources->set_texture(depth_texture_name_, fb_->texture_depth_buffer);
        }
        CHK(glViewport(0, 0, width, height));
    }
}
