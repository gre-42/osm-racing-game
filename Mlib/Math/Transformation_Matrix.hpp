#pragma once
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <cmath>
#include <iostream>

namespace Mlib {

template <class TData>
class TransformationMatrix {
public:
    TransformationMatrix(const FixedArray<TData, 3, 3>& R, const FixedArray<float, 3>& t)
    : R_{R},
      t_{t}
    {}

    explicit TransformationMatrix(const FixedArray<TData, 4, 4>& m)
    : R_{R3_from_4x4(m)},
      t_{t3_from_4x4(m)}
    {}

    FixedArray<TData, 3> operator * (const FixedArray<TData, 3>& rhs) const {
        return dot1d(R_, rhs) + t_;
    }

    FixedArray<TData, 3> rotate(const FixedArray<TData, 3>& rhs) const {
        return dot1d(R_, rhs);
    }

    const FixedArray<TData, 3, 3>& R() const {
        return R_;
    }

    const FixedArray<TData, 3>& t() const {
        return t_;
    }

    const FixedArray<TData, 4, 4> affine() const {
        return assemble_homogeneous_4x4(R_, t_);
    }
private:
    FixedArray<TData, 3, 3> R_;
    FixedArray<TData, 3> t_;
};

}
