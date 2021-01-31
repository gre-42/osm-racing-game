#include "Lightmap_Logic.hpp"
#include <Mlib/Log.hpp>
#include <Mlib/Render/Aggregate_Array_Renderer.hpp>
#include <Mlib/Render/Array_Instances_Renderer.hpp>
#include <Mlib/Render/CHK.hpp>
#include <Mlib/Render/Instance_Handles/Frame_Buffer.hpp>
#include <Mlib/Render/Instance_Handles/RenderGuards.hpp>
#include <Mlib/Render/Render_Config.hpp>
#include <Mlib/Render/Render_Logics/Resource_Update_Cycle.hpp>
#include <Mlib/Render/Rendered_Scene_Descriptor.hpp>
#include <Mlib/Render/Rendering_Resources.hpp>
#include <Mlib/Render/Viewport_Guard.hpp>

using namespace Mlib;

LightmapLogic::LightmapLogic(
    RenderLogic& child_logic,
    ResourceUpdateCycle update_cycle,
    const std::string& light_node_name,
    const std::string& black_node_name,
    bool with_depth_texture)
: child_logic_{child_logic},
  rendering_context_{RenderingContextStack::rendering_context()},
  update_cycle_{update_cycle},
  light_node_name_{light_node_name},
  black_node_name_{black_node_name},
  with_depth_texture_{with_depth_texture}
{}

LightmapLogic::~LightmapLogic() {
    if (fbs_ != nullptr) {
        rendering_context_.rendering_resources->delete_texture("lightmap_color." + light_node_name_);
        rendering_context_.rendering_resources->delete_vp("lightmap_color." + light_node_name_);
        if (with_depth_texture_) {
            rendering_context_.rendering_resources->delete_texture("lightmap_depth" + light_node_name_);
            rendering_context_.rendering_resources->delete_vp("lightmap_depth" + light_node_name_);
        }
    }
}

void LightmapLogic::render(
    int width,
    int height,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    RenderResults* render_results,
    const RenderedSceneDescriptor& frame_id)
{
    LOG_FUNCTION("LightmapLogic::render");
    if (frame_id.external_render_pass.pass == ExternalRenderPassType::LIGHTMAP_TO_TEXTURE) {
        throw std::runtime_error("LightmapLogic received lightmap rendering");
    }
    if ((fbs_ == nullptr) || (update_cycle_ == ResourceUpdateCycle::ALWAYS)) {
        GLsizei lightmap_width = black_node_name_.empty()
            ? render_config.scene_lightmap_width
            : render_config.black_lightmap_width;
        GLsizei lightmap_height = black_node_name_.empty()
            ? render_config.scene_lightmap_height
            : render_config.black_lightmap_height;
        ViewportGuard vg{0, 0, lightmap_width, lightmap_height};
        RenderedSceneDescriptor light_rsd{.external_render_pass = {ExternalRenderPassType::LIGHTMAP_TO_TEXTURE, black_node_name_}, .time_id = 0, .light_node_name = light_node_name_};
        if (fbs_ == nullptr) {
            fbs_ = std::make_unique<FrameBufferMsaa>();
        }
        fbs_->configure({
            .width = lightmap_width,
            .height = lightmap_height,
            .with_depth_texture = with_depth_texture_,
            .nsamples_msaa = render_config.lightmap_nsamples_msaa});
        {
            RenderToFrameBufferGuard rfg{*fbs_};
            RenderingContextGuard rrg{rendering_context_};
            AggregateRendererGuard small_aggregate_array_renderer{std::make_shared<AggregateArrayRenderer>()};
            InstancesRendererGuard small_instances_renderer{std::make_shared<ArrayInstancesRenderer>()};
            child_logic_.render(lightmap_width, lightmap_height, render_config, scene_graph_config, render_results, light_rsd);
            // VectorialPixels<float, 3> vpx{ArrayShape{size_t(lightmap_width), size_t(lightmap_height)}};
            // CHK(glReadPixels(0, 0, lightmap_width, lightmap_height, GL_RGB, GL_FLOAT, vpx->flat_iterable().begin()));
            // PpmImage::from_float_rgb(vpx.to_array()).save_to_file("/tmp/lightmap.ppm");
        }

        rendering_context_.rendering_resources->set_texture("lightmap_color." + light_node_name_, fbs_->fb.texture_color);
        rendering_context_.rendering_resources->set_vp("lightmap_color." + light_node_name_, vp());
        if (with_depth_texture_) {
            rendering_context_.rendering_resources->set_texture("lightmap_depth" + light_node_name_, fbs_->fb.texture_depth);
            rendering_context_.rendering_resources->set_vp("lightmap_depth" + light_node_name_, vp());
        }
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

const TransformationMatrix<float, 3>& LightmapLogic::iv() const {
    return child_logic_.iv();
}

bool LightmapLogic::requires_postprocessing() const {
    return child_logic_.requires_postprocessing();
}
