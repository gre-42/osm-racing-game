#include "Osm_Triangle_Lists.hpp"
#include <Mlib/Geometry/Mesh/Triangle_List.hpp>
#include <Mlib/Render/Rendering_Context.hpp>
#include <Mlib/Render/Rendering_Resources.hpp>
#include <Mlib/Render/Resources/Osm_Map_Resource/Entrance_Type.hpp>
#include <Mlib/Render/Resources/Osm_Map_Resource/Osm_Resource_Config.hpp>
#include <Mlib/Render/Resources/Osm_Map_Resource/Road_Type.hpp>

using namespace Mlib;

void RoadPropertiesTriangleList::insert(const RoadProperties& rp, const std::shared_ptr<TriangleList>& lst) {
    if (!lst_.insert({rp, lst}).second) {
        throw std::runtime_error("Could not insert triangle list");
    }
}

const std::shared_ptr<TriangleList>& RoadPropertiesTriangleList::operator [] (RoadProperties road_properties) const {
    const std::shared_ptr<TriangleList>* result = nullptr;
    for (const auto& l : lst_) {
        if (l.first.type != road_properties.type) {
            continue;
        }
        if (l.first.nlanes <= road_properties.nlanes) {
            result = &l.second;
        }
    }
    if (result == nullptr) {
        throw std::runtime_error("Could not find matching triangle list for properties " + (std::string)road_properties);
    }
    return *result;
}

const std::map<RoadProperties, std::shared_ptr<TriangleList>>& RoadPropertiesTriangleList::map() const {
    return lst_;
}

void RoadTypeTriangleList::insert(RoadType road_type, const std::shared_ptr<TriangleList>& lst) {
    if (!lst_.insert({road_type, lst}).second) {
        throw std::runtime_error("Could not insert triangle list");
    }
}
const std::shared_ptr<TriangleList>& RoadTypeTriangleList::operator [] (RoadType road_type) const {
    auto it = lst_.find(road_type);
    if (it == lst_.end()) {
        throw std::runtime_error("Could not find road with type " + road_type_to_string(road_type));
    }
    return it->second;
}
const std::map<RoadType, std::shared_ptr<TriangleList>>& RoadTypeTriangleList::map() const {
    return lst_;
}

OsmTriangleLists::OsmTriangleLists(const OsmResourceConfig& config)
{
    tl_terrain = std::make_shared<TriangleList>("terrain", Material{
        .dirt_texture = config.dirt_texture,
        .occluded_type = OccludedType::LIGHT_MAP_COLOR,
        .occluder_type = OccluderType::WHITE,
        .aggregate_mode = config.blend_street ? AggregateMode::ONCE : AggregateMode::OFF,
        .specularity = {0.f, 0.f, 0.f},
        .draw_distance_noperations = 1000}.compute_color_mode());
    tl_terrain_visuals = std::make_shared<TriangleList>("tl_terrain_visuals", Material{
        .dirt_texture = config.dirt_texture,
        .occluded_type = OccludedType::LIGHT_MAP_COLOR,
        .occluder_type = OccluderType::WHITE,
        .collide = false,
        .aggregate_mode = config.blend_street ? AggregateMode::ONCE : AggregateMode::OFF,
        .specularity = {0.f, 0.f, 0.f},
        .draw_distance_noperations = 1000}.compute_color_mode());
    tl_terrain_street_extrusion = std::make_shared<TriangleList>("terrain_street_extrusion", Material{
        .dirt_texture = config.dirt_texture,
        .occluded_type = OccludedType::LIGHT_MAP_COLOR,
        .occluder_type = OccluderType::WHITE,
        .aggregate_mode = config.blend_street ? AggregateMode::ONCE : AggregateMode::OFF,
        .specularity = {0.f, 0.f, 0.f},
        .draw_distance_noperations = 1000}.compute_color_mode());

    auto primary_rendering_resources = RenderingContextStack::primary_rendering_resources();
    for (auto& t : config.terrain_textures) {
        // BlendMapTexture bt{ .texture_descriptor = {.color = t, .normal = primary_rendering_resources->get_normalmap(t), .anisotropic_filtering_level = anisotropic_filtering_level } };
        BlendMapTexture bt = primary_rendering_resources->get_blend_map_texture(t);
        tl_terrain->material_.textures.push_back(bt);
        tl_terrain_visuals->material_.textures.push_back(bt);
        tl_terrain_street_extrusion->material_.textures.push_back(bt);
    }
    tl_terrain->material_.compute_color_mode();
    tl_terrain_visuals->material_.compute_color_mode();
    tl_terrain_street_extrusion->material_.compute_color_mode();
    for (const auto& s : config.street_crossing_texture) {
        tl_street_crossing.insert(s.first, std::make_shared<TriangleList>(road_type_to_string(s.first), Material{
            .textures = {primary_rendering_resources->get_blend_map_texture(s.second)},
            .occluded_type = OccludedType::LIGHT_MAP_COLOR,
            .occluder_type = OccluderType::WHITE,
            .draw_distance_noperations = 1000}.compute_color_mode()));
    }
    for (const auto& s : config.street_texture) {
        tl_street.insert(s.first, std::make_shared<TriangleList>((std::string)s.first, Material{
            .continuous_blending_z_order = config.blend_street ? 1 : 0,
            .blend_mode = config.blend_street ? BlendMode::CONTINUOUS : BlendMode::OFF,
            .textures = {primary_rendering_resources->get_blend_map_texture(s.second)},
            .occluded_type = OccludedType::LIGHT_MAP_COLOR,
            .occluder_type = OccluderType::WHITE,
            .depth_func_equal = config.blend_street,
            .aggregate_mode = config.blend_street ? AggregateMode::ONCE : AggregateMode::OFF,
            .draw_distance_noperations = 1000}.compute_color_mode())); // mixed_texture: terrain_texture
    }
    WrapMode curb_wrap_mode_s = (config.extrude_curb_amount != 0) || ((config.curb_alpha != 1) && (config.extrude_street_amount != 0)) ? WrapMode::REPEAT : WrapMode::CLAMP_TO_EDGE;
    for (const auto& s : config.curb_street_texture) {
        tl_street_curb.insert(s.first, std::make_shared<TriangleList>(road_type_to_string(s.first), Material{
            .textures = {primary_rendering_resources->get_blend_map_texture(s.second)},
            .occluded_type = OccludedType::LIGHT_MAP_COLOR,
            .occluder_type = OccluderType::WHITE,
            .wrap_mode_s = curb_wrap_mode_s,
            .specularity = OrderableFixedArray{fixed_full<float, 3>((float)(config.extrude_curb_amount == 0))},
            .draw_distance_noperations = 1000}.compute_color_mode())); // mixed_texture: terrain_texture
    }
    for (const auto& s : config.curb2_street_texture) {
        tl_street_curb2.insert(s.first, std::make_shared<TriangleList>(road_type_to_string(s.first), Material{
            .textures = {primary_rendering_resources->get_blend_map_texture(s.second)},
            .occluded_type = OccludedType::LIGHT_MAP_COLOR,
            .occluder_type = OccluderType::WHITE,
            .draw_distance_noperations = 1000}.compute_color_mode())); // mixed_texture: terrain_texture
    }
    for (const auto& s : config.air_curb_street_texture) {
        tl_air_street_curb.insert(s.first, std::make_shared<TriangleList>(road_type_to_string(s.first), Material{
            .textures = {primary_rendering_resources->get_blend_map_texture(s.second)},
            .occluded_type = OccludedType::LIGHT_MAP_COLOR,
            .occluder_type = OccluderType::WHITE,
            .wrap_mode_s = curb_wrap_mode_s,
            .draw_distance_noperations = 1000}.compute_color_mode()));
    }
    tl_air_support = std::make_shared<TriangleList>("air_support", Material{
        .textures = {primary_rendering_resources->get_blend_map_texture(config.air_support_texture)},
        .occluded_type = OccludedType::LIGHT_MAP_COLOR,
        .occluder_type = OccluderType::WHITE,
        .wrap_mode_s = curb_wrap_mode_s,
        .draw_distance_noperations = 1000}.compute_color_mode());
    tl_tunnel_crossing = std::make_shared<TriangleList>("tunnel_crossing", Material{
        .textures = {primary_rendering_resources->get_blend_map_texture(config.tunnel_pipe_texture)},
        .occluded_type = OccludedType::LIGHT_MAP_COLOR,
        .occluder_type = OccluderType::WHITE,
        .specularity = {0.f, 0.f, 0.f},
        .draw_distance_noperations = 1000}.compute_color_mode());
    tl_tunnel_pipe = std::make_shared<TriangleList>("tunnel_pipe", Material{
        .textures = {primary_rendering_resources->get_blend_map_texture(config.tunnel_pipe_texture)},
        .occluded_type = OccludedType::LIGHT_MAP_COLOR,
        .occluder_type = OccluderType::WHITE,
        .specularity = {0.f, 0.f, 0.f},
        .draw_distance_noperations = 1000}.compute_color_mode());
    tl_tunnel_bdry = std::make_shared<TriangleList>("tunnel_bdry", Material());
    tl_entrance[EntranceType::TUNNEL] = std::make_shared<TriangleList>("tunnel_entrance", Material());
    tl_entrance[EntranceType::BRIDGE] = std::make_shared<TriangleList>("bridge_entrance", Material());
    entrances[EntranceType::TUNNEL];
    entrances[EntranceType::BRIDGE];
    tl_water = std::make_shared<TriangleList>("water", Material{
        .textures = {primary_rendering_resources->get_blend_map_texture(config.water_texture)},
        .collide = false,
        .draw_distance_noperations = 1000}.compute_color_mode());
}

OsmTriangleLists::~OsmTriangleLists()
{}

#define INSERT(a) a->triangles_.insert(a->triangles_.end(), other.a->triangles_.begin(), other.a->triangles_.end())
#define INSERT2(a) for (const auto& s : other.a.map()) a[s.first]->triangles_.insert(a[s.first]->triangles_.end(), s.second->triangles_.begin(), s.second->triangles_.end())
void OsmTriangleLists::insert(const OsmTriangleLists& other) {
    INSERT(tl_terrain);
    INSERT(tl_terrain_visuals);
    INSERT(tl_terrain_street_extrusion);
    INSERT2(tl_street_crossing);
    INSERT2(tl_street);
    INSERT2(tl_street_curb);
    INSERT2(tl_street_curb2);
    INSERT2(tl_air_street_curb);
    INSERT(tl_air_support);
    INSERT(tl_tunnel_crossing);
    INSERT(tl_tunnel_pipe);
    INSERT(tl_tunnel_bdry);
}
#undef INSERT
#undef INSERT2

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_street_wo_curb() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_street() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{
        tl_air_support};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb2.map()) {res.push_back(e.second);}
    for (const auto& e : tl_air_street_curb.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_smooth() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{
        tl_terrain,
        tl_terrain_visuals,
        tl_terrain_street_extrusion};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb2.map()) {res.push_back(e.second);}
    for (const auto& e : tl_air_street_curb.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_no_backfaces() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{
        tl_terrain,
        tl_terrain_visuals,
        tl_terrain_street_extrusion,
        tl_air_support,
        tl_tunnel_crossing};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb2.map()) {res.push_back(e.second);}
    for (const auto& e : tl_air_street_curb.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_wo_subtraction_and_water() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{
        tl_terrain,
        tl_terrain_visuals,
        tl_terrain_street_extrusion,
        tl_air_support,
        tl_tunnel_crossing,
        tl_tunnel_pipe};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb2.map()) {res.push_back(e.second);}
    for (const auto& e : tl_air_street_curb.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_wo_subtraction_w_water() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{
        tl_terrain,
        tl_terrain_visuals,
        tl_terrain_street_extrusion,
        tl_air_support,
        tl_tunnel_crossing,
        tl_tunnel_pipe,
        tl_water};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb2.map()) {res.push_back(e.second);}
    for (const auto& e : tl_air_street_curb.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_all() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{
        tl_terrain,
        tl_terrain_visuals,
        tl_terrain_street_extrusion,
        tl_air_support,
        tl_tunnel_crossing,
        tl_tunnel_pipe,
        tl_tunnel_bdry};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb2.map()) {res.push_back(e.second);}
    for (const auto& e : tl_air_street_curb.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_with_vertex_normals() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{
        tl_terrain,
        tl_terrain_visuals,
        tl_tunnel_crossing};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_no_grass() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{
        tl_air_support,
        tl_tunnel_crossing,
        tl_tunnel_bdry};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb.map()) {res.push_back(e.second);}
    for (const auto& e : tl_street_curb2.map()) {res.push_back(e.second);}
    for (const auto& e : tl_air_street_curb.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_curb_only() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{};
    for (const auto& e : tl_street_curb.map()) {res.push_back(e.second);}
    return res;
}

std::list<std::shared_ptr<TriangleList>> OsmTriangleLists::tls_crossing_only() const {
    auto res = std::list<std::shared_ptr<TriangleList>>{};
    for (const auto& e : tl_street_crossing.map()) {res.push_back(e.second);}
    return res;
}

#define INSERT(a) result.insert(result.end(), a->triangles_.begin(), a->triangles_.end())
#define INSERT2(a) for (const auto& e : a.map()) { \
    result.insert( \
        result.end(), \
        e.second->triangles_.begin(), \
        e.second->triangles_.end()); \
}
std::list<FixedArray<ColoredVertex, 3>> OsmTriangleLists::hole_triangles() const {
    std::list<FixedArray<ColoredVertex, 3>> result;
    INSERT2(tl_street_crossing);
    INSERT2(tl_street);
    INSERT2(tl_street_curb);
    INSERT2(tl_street_curb2);
    INSERT(tl_entrance.at(EntranceType::TUNNEL));
    INSERT(tl_entrance.at(EntranceType::BRIDGE));
    return result;
}

std::list<FixedArray<ColoredVertex, 3>> OsmTriangleLists::street_triangles() const {
    std::list<FixedArray<ColoredVertex, 3>> result;
    INSERT2(tl_street);
    return result;
}

#undef INSERT
#undef INSERT2
