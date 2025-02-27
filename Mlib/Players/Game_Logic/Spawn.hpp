#pragma once
#include <Mlib/Geometry/Intersection/Bvh_Fwd.hpp>
#include <Mlib/Scene_Precision.hpp>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <vector>

namespace Mlib {

template <class T>
class DanglingRef;
class Bystanders;
struct SpawnPoint;
class VehicleSpawner;
class VehicleSpawners;
class Player;
class Players;
class GameLogic;
struct GameLogicConfig;
class Scene;
class SceneNode;
class TeamDeathmatch;
class DeleteNodeMutex;
template <class TDir, class TPos, size_t n>
class TransformationMatrix;

class Spawn {
    friend Bystanders;
    friend TeamDeathmatch;
    friend GameLogic;
public:
    explicit Spawn(
        VehicleSpawners& vehicle_spawners,
        Players& players,
        GameLogicConfig& cfg,
        DeleteNodeMutex& delete_node_mutex,
        Scene& scene);
    ~Spawn();
    void set_spawn_points(
        const TransformationMatrix<float, ScenePos, 3>& absolute_model_matrix,
        const std::list<SpawnPoint>& spawn_points);
    void respawn_all_players();
    void spawn_player_during_match(VehicleSpawner& spawner);
private:
    void spawn_at_spawn_point(
        VehicleSpawner& spawner,
        const SpawnPoint& sp);
    std::vector<SpawnPoint*> shuffled_spawn_points();
    std::vector<SpawnPoint> spawn_points_;
    std::vector<std::unique_ptr<Bvh<CompressedScenePos, 3, const SpawnPoint*>>> spawn_points_bvh_split_;
    std::unique_ptr<Bvh<CompressedScenePos, 3, const SpawnPoint*>> spawn_points_bvh_singular_;
    VehicleSpawners& vehicle_spawners_;
    Players& players_;
    GameLogicConfig& cfg_;
    DeleteNodeMutex& delete_node_mutex_;
    Scene& scene_;
    size_t nspawns_;
    size_t ndelete_;
};

}
