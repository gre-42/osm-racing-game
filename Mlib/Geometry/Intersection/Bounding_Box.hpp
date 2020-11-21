#pragma once
#include <Mlib/Math/Fixed_Math.hpp>
#include <Mlib/Stats/Min_Max.hpp>
#include <iosfwd>
#include <string>

#pragma GCC push_options
#pragma GCC optimize ("O3")

namespace Mlib {

template <class TData, size_t tndim>
class BoundingSphere;

template <class TData, size_t tndim>
class BoundingBox {
public:
    BoundingBox()
    : min_(INFINITY),
      max_(-INFINITY)
    {}
    BoundingBox(const FixedArray<TData, tndim>& min, const FixedArray<TData, tndim>& max)
    : min_{min},
      max_{max}
    {}
    BoundingBox(const FixedArray<TData, tndim>& point)
    : min_{point},
      max_{point}
    {}
    template <size_t tnpoints>
    BoundingBox(const FixedArray<FixedArray<TData, tndim>, tnpoints>& points)
    : BoundingBox{points.flat_begin(), points.flat_end()}
    {}
    template <class TIterable>
    explicit BoundingBox(
        const TIterable& iterable_begin,
        const TIterable& iterable_end)
    : BoundingBox()
    {
        for(auto it = iterable_begin; it != iterable_end; ++it) {
            extend(*it);
        }
    }
    bool intersects(const BoundingBox& other) const {
        return all(max_ >= other.min_) && all(min_ <= other.max_);
    }
    bool intersects(const BoundingSphere<TData, tndim>& sphere) const {
        return all(sphere.center() >= min_ - sphere.radius()) &&
               all(sphere.center() <= max_ + sphere.radius());
    }
    void extend(const BoundingBox& other) {
        min_ = minimum(min_, other.min_);
        max_ = maximum(max_, other.max_);
    }
    FixedArray<TData, tndim> size() {
        return max_ - min_;
    }
    void print(std::ostream& ostr, size_t rec = 0) const {
        std::string indent(rec, ' ');
        ostr << indent << "bounds " << min_ << " -- " << max_ << std::endl;
    }
    inline const FixedArray<TData, tndim>& min() const {
        return min_;
    }
    inline const FixedArray<TData, tndim>& max() const {
        return max_;
    }
private:
    FixedArray<TData, tndim> min_;
    FixedArray<TData, tndim> max_;
};

template <class TData, size_t tndim>
std::ostream& operator << (std::ostream& ostr, const BoundingBox<TData, tndim>& bounding_box) {
    bounding_box.print(ostr);
    return ostr;
}

}

#pragma GCC pop_options
