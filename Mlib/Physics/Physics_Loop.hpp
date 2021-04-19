#pragma once
#include <atomic>
#include <functional>
#include <thread>

namespace Mlib {

class PhysicsIteration;
struct PhysicsEngineConfig;
class SetFps;

class PhysicsLoop {
public:
    PhysicsLoop(
        PhysicsIteration& physics_iteration,
        const PhysicsEngineConfig& physics_cfg,
        SetFps& set_fps,
        size_t nframes = SIZE_MAX,
        const std::function<std::function<void()>(std::function<void()>)>& run_in_background = [](std::function<void()> f){return f;});
    ~PhysicsLoop();
    void stop_and_join();
    //! Useful if nframes != SIZE_MAX
    void join();
    void wait_until_paused_and_delete_scheduled_advance_times();
private:
    std::atomic_bool exit_physics_;
    std::atomic_bool idle_;
    SetFps& set_fps_;
    PhysicsIteration& physics_iteration_;
    std::thread physics_thread_;
};

}
