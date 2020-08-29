#pragma once
#include <Mlib/Array/Fixed_Array.hpp>
#include <Mlib/Memory/Destruction_Observer.hpp>
#include <Mlib/Physics/Advance_Time.hpp>
#include <Mlib/Scene_Graph/Transformation/Absolute_Movable.hpp>
#include <memory>

namespace Mlib {

class AdvanceTimes;
class SceneNode;

class KeepOffsetMovable: public DestructionObserver, public AbsoluteMovable, public AdvanceTime {
public:
    KeepOffsetMovable(
        AdvanceTimes& advance_times,
        SceneNode* followed_node,
        AbsoluteMovable* followed,
        const FixedArray<float, 3>& offset);
    virtual void advance_time(float dt) override;
    virtual void set_absolute_model_matrix(const FixedArray<float, 4, 4>& absolute_model_matrix) override;
    virtual FixedArray<float, 4, 4> get_new_absolute_model_matrix() const override;
    virtual void notify_destroyed(void* obj) override;

private:
    AdvanceTimes& advance_times_;
    SceneNode* followed_node_;
    AbsoluteMovable* followed_;
    FixedArray<float, 3> offset_;
    FixedArray<float, 3> position_;
    FixedArray<float, 3, 3> rotation_;
};

}
