#include "Visual_Movable_3rd_Logger.hpp"
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Physics/Containers/Advance_Times.hpp>
#include <Mlib/Render/Text/Renderable_Text.hpp>
#include <Mlib/Scene_Graph/Scene_Node.hpp>
#include <sstream>

using namespace Mlib;

VisualMovable3rdLogger::VisualMovable3rdLogger(
    RenderLogic& scene_logic,
    SceneNode& scene_node,
    AdvanceTimes& advance_times,
    Loggable* logged,
    unsigned int log_components,
    const std::string& ttf_filename,
    const FixedArray<float, 2>& offset,
    float font_height_pixels,
    float line_distance_pixels)
: scene_logic_{scene_logic},
  scene_node_{scene_node},
  advance_times_{advance_times},
  logged_{logged},
  log_components_{log_components},
  ttf_filename_{ttf_filename},
  offset_{offset},
  font_height_pixels_{font_height_pixels},
  line_distance_pixels_{line_distance_pixels}
{
    scene_node.add_destruction_observer(this);
}

VisualMovable3rdLogger::~VisualMovable3rdLogger()
{}

void VisualMovable3rdLogger::notify_destroyed(void* destroyed_object) {
    advance_times_.schedule_delete_advance_time(this);
}

void VisualMovable3rdLogger::advance_time(float dt) {
    std::stringstream sstr;
    logged_->log(sstr, log_components_);
    text_ = sstr.str();
}

void VisualMovable3rdLogger::initialize(GLFWwindow* window) {
    if (renderable_text_ != nullptr) {
        throw std::runtime_error("Multiple calls to VisualMovable3rdLogger::initialize");
    }
    renderable_text_.reset(new RenderableText{ttf_filename_, font_height_pixels_});
}

void VisualMovable3rdLogger::render(
    int width,
    int height,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    RenderResults* render_results,
    const RenderedSceneDescriptor& frame_id)
{
    FixedArray<float, 3> node_pos = t3_from_4x4(scene_node_.absolute_model_matrix());
    auto position4 = dot1d(scene_logic_.vp(), homogenized_4(node_pos));
    if (position4(2) > scene_logic_.near_plane()) {
        FixedArray<float, 2> position2{
            position4(0) / position4(3) + offset_(0),
            -position4(1) / position4(3) - offset_(1)};
        auto p2 = (position2 * 0.5f + 0.5f) * FixedArray<float, 2>{(float)width, (float)height};
        renderable_text_->render(p2, text_, width, height, line_distance_pixels_);
    }
}

float VisualMovable3rdLogger::near_plane() const {
    throw std::runtime_error("VisualMovable3rdLogger::requires_postprocessing not implemented");
}

float VisualMovable3rdLogger::far_plane() const {
    throw std::runtime_error("VisualMovable3rdLogger::requires_postprocessing not implemented");
}

const FixedArray<float, 4, 4>& VisualMovable3rdLogger::vp() const {
    throw std::runtime_error("VisualMovable3rdLogger::vp not implemented");
}

bool VisualMovable3rdLogger::requires_postprocessing() const {
    throw std::runtime_error("VisualMovable3rdLogger::requires_postprocessing not implemented");
}
