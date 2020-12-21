#include "Check_Points.hpp"
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Math/Fixed_Rodrigues.hpp>
#include <Mlib/Physics/Containers/Advance_Times.hpp>
#include <Mlib/Physics/Containers/Players.hpp>
#include <Mlib/Scene_Graph/Scene_Node.hpp>
#include <Mlib/Scene_Graph/Transformation/Absolute_Movable.hpp>
#include <Mlib/Time.hpp>

using namespace Mlib;

CheckPoints::CheckPoints(
    const std::string& filename,
    AdvanceTimes& advance_times,
    SceneNode* moving_node,
    AbsoluteMovable* moving,
    const std::vector<SceneNode*>& beacon_nodes,
    Players* players,
    Player* player,
    size_t nth,
    size_t nahead,
    float radius)
: advance_times_{advance_times},
  track_reader_{filename},
  moving_node_{moving_node},
  moving_{moving},
  beacon_nodes_{beacon_nodes},
  players_{players},
  player_{player},
  radius_{radius},
  nth_{nth},
  nahead_{nahead},
  i01_{0}
{
    if (beacon_nodes_.empty()) {
        throw std::runtime_error("Need at least one beacon node");
    }
    if (nth_ == 0) {
        throw std::runtime_error("nth is 0");
    }
    moving_node_->add_destruction_observer(this);
    advance_time(0);
}

void CheckPoints::advance_time(float dt) {
    bool just_started = checkpoints_ahead_.empty();

    if (just_started) {
        start_time_ = std::chrono::steady_clock::now();
    }

    while ((checkpoints_ahead_.size() < nahead_) && (!track_reader_.eof())) {
        for(size_t i = 0; i < nth_; ++i) {
            float time;
            FixedArray<float, 3> position;
            FixedArray<float, 3> rotation;
            if (track_reader_.read(time, position, rotation) &&
                (i == nth_ - 1))
            {
                checkpoints_ahead_.push_back({.position = position, .rotation = rotation});
                beacon_nodes_[i01_]->set_relative_pose(position, rotation, 1);
                i01_ = (i01_ + 1) % beacon_nodes_.size();
            }
        }
    }

    if (!checkpoints_ahead_.empty() &&
        (sum(squared(t3_from_4x4(moving_->get_new_absolute_model_matrix()) - checkpoints_ahead_.front().position)) < squared(radius_)))
    {
        checkpoints_ahead_.pop_front();
    }

    if (!just_started && checkpoints_ahead_.empty() && track_reader_.eof())
    {
        std::chrono::duration<float> elapsed_time{std::chrono::steady_clock::now() - start_time_};
        std::cerr << "Elapsed time: " << format_minutes_seconds(elapsed_time.count()) << std::endl;
        players_->notify_lap_time(player_, elapsed_time.count());
        track_reader_.restart();
        advance_time(0);
    }
}

void CheckPoints::notify_destroyed(void* obj) {
    moving_node_ = nullptr;
    moving_ = nullptr;
    advance_times_.schedule_delete_advance_time(this);
}
