#pragma once
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <cmath>
#include <iostream>

#ifdef __GNUC__
    #pragma GCC push_options
    #pragma GCC optimize ("O3")
#endif

namespace Mlib {

template <class TData, size_t n>
class TransformationMatrix {
public:
    static inline TransformationMatrix identity() {
        return TransformationMatrix{
            fixed_identity_array<TData, n>(),
            fixed_zeros<TData, n>()};
    }

    static inline TransformationMatrix inverse(const FixedArray<TData, n, n>& R, const FixedArray<TData, n>& t) {
        TransformationMatrix result;
        invert_t_R(t, R, result.t_, result.R_);
        return result;
    }

    inline TransformationMatrix()
    {}

    inline TransformationMatrix(const FixedArray<TData, n, n>& R, const FixedArray<TData, n>& t)
    : R_{R},
      t_{t}
    {}

    inline explicit TransformationMatrix(const FixedArray<TData, n+1, n+1>& m)
    : R_{R_from_NxN(m)},
      t_{t_from_NxN(m)}
    {}

    inline FixedArray<TData, n> transform(const FixedArray<TData, n>& rhs) const {
        return dot1d(R_, rhs) + t_;
    }

    inline TransformationMatrix operator * (const TransformationMatrix& rhs) const {
        return TransformationMatrix{
            dot2d(R_, rhs.R_),
            dot1d(R_, rhs.t_) + t_};
    }

    inline FixedArray<TData, n> rotate(const FixedArray<TData, n>& rhs) const {
        return dot1d(R_, rhs);
    }

    inline const FixedArray<TData, n, n>& R() const {
        return R_;
    }

    inline const FixedArray<TData, n>& t() const {
        return t_;
    }

    inline FixedArray<TData, n, n>& R() {
        return R_;
    }

    inline FixedArray<TData, n>& t() {
        return t_;
    }

    inline const FixedArray<TData, n+1, n+1> affine() const {
        return assemble_homogeneous_NxN(R_, t_);
    }

    inline TransformationMatrix inverted() const {
        return inverse(R_, t_);
    }

    inline float get_scale2() const {
        return sum(squared(R_)) / n;
    }

    inline float get_scale() const {
        return std::sqrt(get_scale2());
    }

    inline TransformationMatrix inverted_scaled() const {
        return inverse(R_ / get_scale2(), t_);
    }
    
    void pre_scale(const TData& f) {
        R_ *= f;
        t_ *= f;
    }

    TransformationMatrix pre_scaled(const TData& f) const {
        return TransformationMatrix{R_ * f, t_ * f};
    }

    template <class TResultData>
    TransformationMatrix<TResultData, n> casted() const {
        return TransformationMatrix<TResultData, n>{
            R_.template casted<TResultData>(),
            t_.template casted<TResultData>()};
    }
private:
    FixedArray<TData, n, n> R_;
    FixedArray<TData, n> t_;
};

}

#ifdef __GNUC__
    #pragma GCC pop_options
#endif
