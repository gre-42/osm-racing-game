#pragma once
#include <Mlib/Array/Array_Forward.hpp>

namespace Mlib {

template <class TData, size_t tsize>
class TransformationMatrix;
class SceneNode;

class RelativeMovable {
public:
    virtual ~RelativeMovable() = default;
    virtual void set_initial_relative_model_matrix(const TransformationMatrix<float, 3>& relative_model_matrix) = 0;
    virtual void set_updated_relative_model_matrix(const TransformationMatrix<float, 3>& relative_model_matrix) = 0;
    virtual void set_absolute_model_matrix(const TransformationMatrix<float, 3>& absolute_model_matrix) = 0;
    virtual TransformationMatrix<float, 3> get_new_relative_model_matrix() const = 0;
};

}
