#include "Scene_Selector_Logic.hpp"
#include <Mlib/Geometry/Cameras/Camera.hpp>
#include <Mlib/Layout/IWidget.hpp>
#include <Mlib/Log.hpp>
#include <Mlib/Macro_Executor/Json_Expression.hpp>
#include <Mlib/Macro_Executor/Macro_Line_Executor.hpp>
#include <Mlib/Macro_Executor/Replacement_Parameter.hpp>
#include <Mlib/Render/Key_Bindings/Base_Key_Binding.hpp>
#include <Mlib/Render/Render_Setup.hpp>
#include <Mlib/Render/Text/Renderable_Text.hpp>
#include <Mlib/Render/Ui/Button_Press.hpp>
#include <Mlib/Render/Ui/List_View_Orientation.hpp>
#include <Mlib/Render/Ui/List_View_String_Drawer.hpp>
#include <Mlib/Threads/Containers/Thread_Safe_String.hpp>

using namespace Mlib;

SceneEntry::SceneEntry(const ReplacementParameterAndFilename& rpe)
    : rpe_{ rpe }
    , locals_(std::map<std::string, std::string>{{"id", id()}})
{}

const std::string& SceneEntry::id() const {
    return rpe_.rp.id;
}

const std::string& SceneEntry::name() const {
    return rpe_.rp.title;
}

const std::string& SceneEntry::filename() const {
    return rpe_.filename;
}

const nlohmann::json& SceneEntry::on_before_select() const {
    return rpe_.rp.on_before_select;
}

JsonView SceneEntry::locals() const {
    return JsonView{ locals_ };
}

const ReplacementParameterRequired& SceneEntry::required() const {
    return rpe_.rp.required;
}

bool SceneEntry::operator < (const SceneEntry& other) const {
    return name() < other.name();
}

SceneEntryContents::SceneEntryContents(
    const std::vector<SceneEntry>& scene_entries,
    const MacroLineExecutor& mle)
    : scene_entries_{scene_entries}
    , mle_{mle}
{}

size_t SceneEntryContents::num_entries() const {
    return scene_entries_.size();
}

bool SceneEntryContents::is_visible(size_t index) const {
    const auto& entry = scene_entries_[index];
    for (const auto& r : entry.required().dynamic) {
        if (!mle_.eval<bool>(r, entry.locals())) {
            return false;
        }
    }
    return true;
}

SceneSelectorLogic::SceneSelectorLogic(
    std::string debug_hint,
    std::vector<SceneEntry> scene_files,
    const std::string& ttf_filename,
    std::unique_ptr<IWidget>&& widget,
    const FixedArray<float, 3>& font_color,
    const ILayoutPixels& font_height,
    const ILayoutPixels& line_distance,
    FocusFilter focus_filter,
    MacroLineExecutor mle,
    ThreadSafeString& next_scene_filename,
    ButtonStates& button_states,
    std::atomic_size_t& selection_index,
    const std::function<void()>& on_change)
    : mle_{ std::move(mle) }
    , renderable_text_{ std::make_unique<TextResource>(
        ttf_filename,
        font_color) }
    , scene_files_{ std::move(scene_files) }
    , contents_{ scene_files_, mle_ }
    , widget_{ std::move(widget) }
    , font_height_{ font_height }
    , line_distance_{ line_distance }
    , focus_filter_{ std::move(focus_filter) }
    , next_scene_filename_{ next_scene_filename }
    , list_view_{
        std::move(debug_hint),
        button_states,
        selection_index,
        contents_,
        ListViewOrientation::VERTICAL,
        [this, on_change]() {
            next_scene_filename_ = scene_files_.at(list_view_.selected_element()).filename();
            merge_substitutions();
            on_change();
        } }
{
    mle_.add_observer([this]() {
        list_view_.notify_change_visibility();
        });
}

SceneSelectorLogic::~SceneSelectorLogic() {
    on_destroy.clear();
}

std::optional<RenderSetup> SceneSelectorLogic::try_render_setup(
    const LayoutConstraintParameters& lx,
    const LayoutConstraintParameters& ly,
    const RenderedSceneDescriptor& frame_id) const
{
    return std::nullopt;
}

void SceneSelectorLogic::render_without_setup(
    const LayoutConstraintParameters& lx,
    const LayoutConstraintParameters& ly,
    const RenderConfig& render_config,
    const SceneGraphConfig& scene_graph_config,
    RenderResults* render_results,
    const RenderedSceneDescriptor& frame_id)
{
    LOG_FUNCTION("SceneSelectorLogic::render");
    auto ew = widget_->evaluate(lx, ly, YOrientation::AS_IS, RegionRoundMode::ENABLED);
    ListViewStringDrawer drawer{
        ListViewOrientation::VERTICAL,
        *renderable_text_,
        font_height_,
        line_distance_,
        *ew,
        ly,
        [this](size_t index) {return scene_files_.at(index).name();}};
    list_view_.render_and_handle_input(lx, ly, drawer);
    drawer.render();
}

FocusFilter SceneSelectorLogic::focus_filter() const {
    return focus_filter_;
}

void SceneSelectorLogic::merge_substitutions() const {
    const auto& element = scene_files_.at(list_view_.selected_element());
    const auto& on_before_select = element.on_before_select();
    if (!on_before_select.is_null()) {
        mle_(on_before_select, nullptr, nullptr);
    }
}

void SceneSelectorLogic::print(std::ostream& ostr, size_t depth) const {
    ostr << std::string(depth, ' ') << "SceneSelectorLogic\n";
}
