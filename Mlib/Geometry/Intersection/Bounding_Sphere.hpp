#pragma once
#include <Mlib/Array/Fixed_Array.hpp>
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Math/Math.hpp>
#include <Mlib/Stats/Min_Max.hpp>

#pragma GCC push_options
#pragma GCC optimize ("O3")

namespace Mlib {

template <class TData, size_t tndim>
class PlaneNd;

template <class TData, size_t tndim>
class BoundingSphere {
public:
    BoundingSphere(
        const FixedArray<TData, tndim>& center,
        const TData& radius)
    : center_{center},
      radius_{radius}
    {}
    explicit BoundingSphere(const FixedArray<FixedArray<TData, tndim>, 2>& line)
    : BoundingSphere{line.flat_begin(), line.flat_end()}
    {}
    explicit BoundingSphere(const FixedArray<FixedArray<TData, tndim>, tndim>& triangle)
    : BoundingSphere{triangle.flat_begin(), triangle.flat_end()}
    {}
    template <class TIterable>
    explicit BoundingSphere(
        const TIterable& iterable_begin,
        const TIterable& iterable_end)
    {
        size_t nelements = 0;
        center_ = 0;
        for(auto it = iterable_begin; it != iterable_end; ++it) {
            center_ += *it;
            ++nelements;
        }
        if (nelements == 0) {
            throw std::runtime_error("Bounding sphere received no elements");
        }
        center_ /= nelements;
        radius_ = 0;
        for(auto it = iterable_begin; it != iterable_end; ++it) {
            radius_ = std::max(radius_, sum(squared(*it - center_)));
        }
        radius_ = std::sqrt(radius_);
    }
    bool intersects(const BoundingSphere& other) const {
        return sum(squared(other.center_ - center_)) <= squared(other.radius_ + radius_);
    }
    bool intersects(const PlaneNd<TData, tndim>& plane) const {
        TData dist = dot0d(plane.normal_, center_) + plane.intercept_;
        return std::abs(dist) <= radius_;
    }
    inline const FixedArray<TData, tndim>& center() const {
        return center_;
    }
    inline const TData& radius() const {
        return radius_;
    }
private:
    FixedArray<TData, tndim> center_;
    TData radius_;
};

}

#pragma GCC pop_options
