#include <Mlib/Arg_Parser.hpp>
#include <Mlib/Render/Gl_Context_Guard.hpp>
#include <Mlib/Render/Render2.hpp>
#include <Mlib/Render/Render_Logics/Lambda_Render_Logic.hpp>
#include <Mlib/Render/Rendering_Context.hpp>
#include <Mlib/Scene/Renderable_Scene.hpp>
#include <Mlib/Strings/From_Number.hpp>

using namespace Mlib;

int main(int argc, char** argv) {

    const ArgParser parser(
        "Usage: render_scene_file scene.scn\n"
        "    [--wire_frame]\n"
        "    [--no_cull_faces]\n"
        "    [--fly]\n"
        "    [--rotate]\n"
        "    [--swap_interval <interval>]\n"
        "    [--nsamples_msaa <nsamples>]\n"
        "    [--lightmap_nsamples_msaa <nsamples>]\n"
        "    [--max_distance_small <distance>]\n"
        "    [--aggregate_update_interval <interval>]\n"
        "    [--screen_width <width>]\n"
        "    [--screen_height <height>]\n"
        "    [--scene_lightmap_width <width>]\n"
        "    [--scene_lightmap_height <height>]\n"
        "    [--black_lightmap_width <width>]\n"
        "    [--black_lightmap_height <height>]\n"
        "    [--window_maximized]\n"
        "    [--full_screen]\n"
        "    [--double_buffer]\n"
        "    [--anisotropic_filtering_level <value>]\n"
        "    [--no_normalmaps]\n"
        "    [--no_physics ]\n"
        "    [--physics_dt <dt> ]\n"
        "    [--render_dt <dt> ]\n"
        "    [--print_physics_residual_time]\n"
        "    [--print_render_residual_time]\n"
        "    [--draw_distance_add <value>]\n"
        "    [--far_plane <value>]\n"
        "    [--record_track]\n"
        "    [--damping <x>]\n"
        "    [--stiction_coefficient <x>]\n"
        "    [--friction_coefficient <x>]\n"
        "    [--alpha0 <x>]\n"
        "    [--lateral_stability <x>]\n"
        "    [--max_extra_friction <x>]\n"
        "    [--max_extra_w <x>]\n"
        "    [--longitudinal_friction_steepness <x>]\n"
        "    [--lateral_friction_steepness <x>]\n"
        "    [--no_slip <x>]\n"
        "    [--no_avoid_burnout]\n"
        "    [--wheel_penetration_depth <x>]\n"
        "    [--print_render_fps]\n"
        "    [--vfx]\n"
        "    [--no_depth_fog]\n"
        "    [--low_pass]\n"
        "    [--high_pass]\n"
        "    [--motion_interpolation]\n"
        "    [--no_render]\n"
        "    [--optimize_search_time]\n"
        "    [--plot_bvh]\n"
        "    [--print_gamepad_buttons]\n"
        "    [--show_mouse_cursor]\n"
        "    [--physics_type {version1, tracking_springs, builtin}]\n"
        "    [--resolve_collision_type {penalty, sequential_pulses}]\n"
        "    [--no_bvh]\n"
        "    [--oversampling]\n"
        "    [--bvh_max_size <r>]\n"
        "    [--static_radius <r>]\n"
        "    [--print_search_time]\n"
        "    [--verbose]",
        {"--wire_frame",
         "--no_cull_faces",
         "--fly",
         "--rotate",
         "--no_physics",
         "--window_maximized",
         "--full_screen",
         "--double_buffer",
         "--no_normalmaps",
         "--print_physics_residual_time",
         "--print_render_residual_time",
         "--print_render_fps",
         "--single_threaded",
         "--no_vfx",
         "--no_depth_fog",
         "--low_pass",
         "--high_pass",
         "--motion_interpolation",
         "--no_render",
         "--optimize_search_time",
         "--plot_bvh",
         "--no_bvh",
         "--record_track",
         "--print_gamepad_buttons",
         "--show_mouse_cursor",
         "--no_slip",
         "--no_avoid_burnout",
         "--print_search_time",
         "--verbose"},
        {"--swap_interval",
         "--nsamples_msaa",
         "--lightmap_nsamples_msaa",
         "--anisotropic_filtering_level",
         "--max_distance_small",
         "--aggregate_update_interval",
         "--screen_width",
         "--screen_height",
         "--scene_lightmap_width",
         "--scene_lightmap_height",
         "--black_lightmap_width",
         "--black_lightmap_height",
         "--static_radius",
         "--bvh_max_size",
         "--physics_dt",
         "--physics_type",
         "--resolve_collision_type",
         "--oversampling",
         "--render_dt",
         "--damping",
         "--stiction_coefficient",
         "--friction_coefficient",
         "--alpha0",
         "--lateral_stability",
         "--max_extra_w",
         "--max_extra_friction",
         "--longitudinal_friction_steepness",
         "--lateral_friction_steepness",
         "--wheel_penetration_depth",
         "--draw_distance_add",
         "--far_plane"});
    try {
        const auto args = parser.parsed(argc, argv);

        args.assert_num_unamed(1);
        std::string main_scene_filename = args.unnamed_value(0);

        size_t num_renderings;
        RenderConfig render_config{
            .nsamples_msaa = safe_stoi(args.named_value("--nsamples_msaa", "2")),
            .lightmap_nsamples_msaa = safe_stoi(args.named_value("--lightmap_nsamples_msaa", "4")),
            .cull_faces = !args.has_named("--no_cull_faces"),
            .wire_frame = args.has_named("--wire_frame"),
            .window_title = main_scene_filename,
            .screen_width = safe_stoi(args.named_value("--screen_width", "640")),
            .screen_height = safe_stoi(args.named_value("--screen_height", "480")),
            .scene_lightmap_width = safe_stoi(args.named_value("--scene_lightmap_width", "2048")),
            .scene_lightmap_height = safe_stoi(args.named_value("--scene_lightmap_height", "2048")),
            .black_lightmap_width = safe_stoi(args.named_value("--black_lightmap_width", "512")),
            .black_lightmap_height = safe_stoi(args.named_value("--black_lightmap_height", "512")),
            .motion_interpolation = args.has_named("--motion_interpolation"),
            .window_maximized = args.has_named("--window_maximized"),
            .full_screen = args.has_named("--full_screen"),
            .double_buffer = args.has_named("--double_buffer"),
            .anisotropic_filtering_level = safe_stou(args.named_value("--anisotropic_filtering_level", "16")),
            .normalmaps = !args.has_named("--no_normalmaps"),
            .show_mouse_cursor = args.has_named("--show_mouse_cursor"),
            .swap_interval = safe_stoi(args.named_value("--swap_interval", "1")),
            .background_color = {0.68f, 0.85f, 1.f},
            .print_fps = args.has_named("--print_render_fps"),
            .print_residual_time = args.has_named("--print_render_residual_time"),
            .dt = safe_stof(args.named_value("--render_dt", "0.01667")),
            .draw_distance_add = safe_stof(args.named_value("--draw_distance_add", "INFINITY"))};
        // Declared as first class to let destructors of other classes succeed.
        Render2 render2{
            num_renderings,
            nullptr,
            render_config};
        
        render2.print_hardware_info();

        ButtonStates button_states;
        UiFocus ui_focus;
        SubstitutionString substitutions;
        std::map<std::string, size_t> selection_ids;
        // FifoLog fifo_log{10 * 1000};

        while (!render2.window_should_close()) {
            num_renderings = SIZE_MAX;
            ui_focus.n_submenus = 0;

            SceneConfig scene_config;

            scene_config.render_config = render_config;

            scene_config.scene_graph_config = SceneGraphConfig{
                .min_distance_small = 1,
                .max_distance_small = safe_stof(args.named_value("--max_distance_small", "1000")),
                .aggregate_update_interval = safe_stoz(args.named_value("--aggregate_update_interval", "100"))};

            scene_config.physics_engine_config = PhysicsEngineConfig{
                .dt = safe_stof(args.named_value("--physics_dt", "0.01667")),
                .print_residual_time = args.has_named("--print_physics_residual_time"),
                .damping = safe_stof(args.named_value("--damping", "0")),
                .stiction_coefficient = safe_stof(args.named_value("--stiction_coefficient", "0.5")),
                .friction_coefficient = safe_stof(args.named_value("--friction_coefficient", "0.5")),
                .alpha0 = safe_stof(args.named_value("--alpha0", "0.1")),
                .avoid_burnout = !args.has_named("--no_avoid_burnout"),
                .no_slip = args.has_named("--no_slip"),
                .lateral_stability = safe_stof(args.named_value("--lateral_stability", "1")),
                .max_extra_friction = safe_stof(args.named_value("--max_extra_friction", "0")),
                .max_extra_w = safe_stof(args.named_value("--max_extra_w", "0")),
                .longitudinal_friction_steepness = safe_stof(args.named_value("--longitudinal_friction_steepness", "5")),
                .lateral_friction_steepness = safe_stof(args.named_value("--lateral_friction_steepness", "7")),
                .wheel_penetration_depth = safe_stof(args.named_value("--wheel_penetration_depth", "0.25")),
                .static_radius = safe_stof(args.named_value("--static_radius", "200")),
                .bvh_max_size = safe_stof(args.named_value("--bvh_max_size", "50")),
                .bvh = !args.has_named("--no_bvh"),
                .physics_type = physics_type_from_string(args.named_value("--physics_type", "builtin")),
                .resolve_collision_type = resolve_collission_type_from_string(args.named_value("--resolve_collision_type", "sequential_pulses")),
                .oversampling = safe_stoz(args.named_value("--oversampling", "2"))};

            SceneNodeResources scene_node_resources;
            {
                std::map<std::string, std::string> sstr{
                    {"PRIMARY_SCENE_FLY", std::to_string(args.has_named("--fly"))},
                    {"PRIMARY_SCENE_ROTATE", std::to_string(args.has_named("--rotate"))},
                    {"PRIMARY_SCENE_PRINT_GAMEPAD_BUTTONS", std::to_string(args.has_named("--print_gamepad_buttons"))},
                    {"PRIMARY_SCENE_DEPTH_FOG", std::to_string(!args.has_named("--no_depth_fog"))},
                    {"PRIMARY_SCENE_LOW_PASS", std::to_string(args.has_named("--low_pass"))},
                    {"PRIMARY_SCENE_HIGH_PASS", std::to_string(args.has_named("--high_pass"))},
                    {"PRIMARY_SCENE_VFX", std::to_string(!args.has_named("--no_vfx"))},
                    {"PRIMARY_SCENE_WITH_DIRTMAP", "1"},
                    {"PRIMARY_SCENE_WITH_SKYBOX", "1"},
                    {"PRIMARY_SCENE_WITH_FLYING_LOGIC", "1"},
                    {"PRIMARY_SCENE_CLEAR_MODE", "color_and_depth"},
                    {"FAR_PLANE", std::to_string(safe_stof(args.named_value("--far_plane", "10000")))},
                    {"IF_RECORD_TRACK", args.has_named("--record_track") ? "" : "#"}
                };
                substitutions.merge(SubstitutionString{sstr});
            }
            std::map<std::string, std::shared_ptr<RenderableScene>> renderable_scenes;
            RenderingContextGuard rrg{scene_node_resources, "primary_rendering_resources", render_config.anisotropic_filtering_level, 0};
            std::string next_scene_filename;
            RegexSubstitutionCache rsc;
            LoadScene load_scene;
            {
                GlContextGuard gcg{ render2.window() };
                load_scene(
                    main_scene_filename,
                    main_scene_filename,
                    next_scene_filename,
                    substitutions,
                    num_renderings,
                    args.has_named("--verbose"),
                    rsc,
                    scene_node_resources,
                    scene_config,
                    render_config,
                    button_states,
                    ui_focus,
                    selection_ids,
                    render2.window(),
                    renderable_scenes);
            }

            if (args.has_named("--print_search_time") ||
                args.has_named("--optimize_search_time") ||
                args.has_named("--plot_bvh"))
            {
                for (const auto& p : renderable_scenes) {
                    if (args.has_named("--print_search_time")) {
                        std::cerr << p.first << " search time" << std::endl;
                    }
                    p.second->print_physics_engine_search_time();
                    if (args.has_named("--optimize_search_time")) {
                        p.second->physics_engine_.rigid_bodies_.optimize_search_time(std::cerr);
                    }
                    if (args.has_named("--plot_bvh")) {
                        p.second->plot_physics_bvh_svg(p.first + "_xz.svg", 0, 2);
                        p.second->plot_physics_bvh_svg(p.first + "_xy.svg", 0, 1);
                    }
                }
            }

            if (!args.has_named("--no_physics") &&
                !args.has_named("--single_threaded"))
            {
                for (const auto& p : renderable_scenes) {
                    p.second->start_physics_loop();
                }
            }

            if (args.has_named("--no_render")) {
                std::cout << "Press enter to exit" << std::endl;
                std::cin.get();
            } else {
                auto rs = renderable_scenes.find("primary_scene");
                if (rs == renderable_scenes.end()) {
                    throw std::runtime_error("Could not find renderable scene with name \"primary_scene\"");
                }
                if (!args.has_named("--single_threaded")) {
                    render2(
                        rs->second->render_logics_,
                        scene_config.scene_graph_config,
                        &button_states);
                } else {
                    LambdaRenderLogic lrl{
                        rs->second->render_logics_,
                        [&]() {
                            for (const auto& p : renderable_scenes) {
                                if (!p.second->physics_set_fps_.paused()) {
                                    p.second->physics_iteration_();
                                }
                            }} };
                    render2(
                        lrl,
                        scene_config.scene_graph_config,
                        &button_states);
                }
                if (!render2.window_should_close()) {
                    ui_focus.focuses = {Focus::SCENE, Focus::LOADING};
                    num_renderings = 1;
                    render2(
                        rs->second->render_logics_,
                        scene_config.scene_graph_config);
                    ui_focus.focuses.pop_back();
                }
            }

            for (const auto& p : renderable_scenes) {
                p.second->stop_and_join();
            }
            main_scene_filename = next_scene_filename;
        }

        // if (!TimeGuard::is_empty(std::this_thread::get_id())) {
        //     std::cerr << "write svg" << std::endl;
        //     TimeGuard::write_svg(std::this_thread::get_id(), "/tmp/events.svg");
        // }
    } catch (const CommandLineArgumentError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
