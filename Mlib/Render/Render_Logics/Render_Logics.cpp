#include "Render_Logics.hpp"
#include <Mlib/Log.hpp>
#include <Mlib/Scene_Graph/Scene_Node.hpp>
#include <stdexcept>

using namespace Mlib;

static std::list<std::pair<SceneNode*, std::shared_ptr<RenderLogic>>>::iterator find_render_logic(SceneNode* node, std::list<std::pair<SceneNode*, std::shared_ptr<RenderLogic>>>& lst) {
    return std::find_if(
        lst.begin(),
        lst.end(),
        [node](const auto& v){ return v.first == node; });
}

RenderLogics::~RenderLogics() {
    std::set<SceneNode*> visited_nodes;
    for(const auto& n : render_logics_) {
        if ((n.first != nullptr) && !visited_nodes.contains(n.first)) {
            visited_nodes.insert(n.first);
            n.first->remove_destruction_observer(this);
        }
    }
}

void RenderLogics::initialize(GLFWwindow* window) {
    if (window_ != nullptr) {
        throw std::runtime_error("RenderLogics::initialize called multiple times");
    }
    window_ = window;
    for(auto& v : to_be_initialized_) {
        v.second->initialize(window);
    }
    to_be_initialized_.clear();
}

void RenderLogics::render(
    int width,
    int height,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    RenderResults* render_results,
    const RenderedSceneDescriptor& frame_id)
{
    LOG_FUNCTION("RenderLogics::render");
    if (window_ == nullptr) {
        throw std::runtime_error("RenderLogics::render called without previous initialize");
    }
    for(const auto& c : render_logics_) {
        c.second->render(width, height, render_config, scene_graph_config, render_results, frame_id);
    }
}

float RenderLogics::near_plane() const {
    throw std::runtime_error("RenderLogics::near_plane not implemented");
}

float RenderLogics::far_plane() const {
    throw std::runtime_error("RenderLogics::far_plane not implemented");
}

const FixedArray<float, 4, 4>& RenderLogics::vp() const {
    throw std::runtime_error("RenderLogics::vp not implemented");
}

bool RenderLogics::requires_postprocessing() const {
    throw std::runtime_error("RenderLogics::requires_postprocessing not implemented");
}

void RenderLogics::prepend(SceneNode* scene_node, const std::shared_ptr<RenderLogic>& render_logic) {
    insert(scene_node, render_logic, true);
}

void RenderLogics::append(SceneNode* scene_node, const std::shared_ptr<RenderLogic>& render_logic) {
    insert(scene_node, render_logic, false);
}

void RenderLogics::insert(SceneNode* scene_node, const std::shared_ptr<RenderLogic>& render_logic, bool prepend) {
    if (window_ == nullptr) {
        to_be_initialized_.push_back(std::make_pair(scene_node, render_logic));
    } else {
        render_logic->initialize(window_);
    }
    if (scene_node != nullptr &&
        (find_render_logic(scene_node, render_logics_) == render_logics_.end()))
    {
        scene_node->add_destruction_observer(this);
    }
    if (prepend) {
        render_logics_.push_front(std::make_pair(scene_node, render_logic));
    } else {
        render_logics_.push_back(std::make_pair(scene_node, render_logic));
    }
}

void RenderLogics::notify_destroyed(void* destroyed_object) {
    size_t nfound = 0;
    while(true) {
        auto del = [this, destroyed_object](std::list<std::pair<SceneNode*, std::shared_ptr<RenderLogic>>>& lst) {
            auto it = find_render_logic((SceneNode*)destroyed_object, lst);
            if (it == lst.end()) {
                return false;
            }
            lst.erase(it);
            return true;
        };
        bool found = false;
        found |= del(to_be_initialized_);
        found |= del(render_logics_);
        if (!found) {
            break;
        }
        ++nfound;
    }
    if (nfound == 0) {
        throw std::runtime_error("Could not find render logic to be deleted");
    }
}
