#pragma once
#include <Mlib/Array/Array.hpp>
#include <Mlib/Math/Fixed_Math.hpp>

namespace Mlib {

Array<float> intrinsic_times_inverse(
    const Array<float>& intrinsic_matrix,
    const Array<float>& inv_rhs);

Array<float> reconstruction_times_inverse(
    const Array<float>& recon_lhs,
    const Array<float>& inv_rhs);

Array<float> inverse_projection_in_reference(
    const Array<float>& reference,
    const Array<float>& b);

Array<float> projection_in_reference(
    const Array<float>& reference,
    const Array<float>& b);

Array<float> reconstruction_in_reference(
    const Array<float>& reference,
    const Array<float>& b);

void invert_t_R(
    const Array<float>& t,
    const Array<float>& R,
    Array<float>& ti,
    Array<float>& Ri);

template <class TData>
inline void invert_t_R(
    const FixedArray<TData, 3>& t,
    const FixedArray<TData, 3, 3>& R,
    FixedArray<TData, 3>& ti,
    FixedArray<TData, 3, 3>& Ri)
{
    Ri = R.T();
    ti = -dot1d(Ri, t);
}

Array<float> t3_from_Nx4(const Array<float>& a, size_t nrows);

Array<float> R3_from_Nx4(const Array<float>& a, size_t nrows);

template <class TData>
inline FixedArray<TData, 4, 4> inverted_scaled_se3(const FixedArray<TData, 4, 4>& m)
{
    auto R = R3_from_4x4(m);
    auto scale2 = sum(squared(R)) / 3;
    return assemble_inverse_homogeneous_4x4(R / scale2, t3_from_4x4(m));
}

inline FixedArray<float, 3> z3_from_4x4(const FixedArray<float, 4, 4>& a) {
    return FixedArray<float, 3>{
        a(0, 2),
        a(1, 2),
        a(2, 2)};
}

inline FixedArray<float, 3> z3_from_3x3(const FixedArray<float, 3, 3>& a) {
    return FixedArray<float, 3>{
        a(0, 2),
        a(1, 2),
        a(2, 2)};
}

inline FixedArray<float, 3> t3_from_4x4(const FixedArray<float, 4, 4>& a) {
    return FixedArray<float, 3>{
        a(0, 3),
        a(1, 3),
        a(2, 3)};
}

template <class TData>
inline FixedArray<TData, 3, 3> R3_from_4x4(const FixedArray<TData, 4, 4>& a)
{
    return FixedArray<TData, 3, 3>{
        a(0, 0), a(0, 1), a(0, 2),
        a(1, 0), a(1, 1), a(1, 2),
        a(2, 0), a(2, 1), a(2, 2)};
}

Array<float> inverted_homogeneous_3x4(const Array<float>& ke);

void homogeneous_to_inverse_t_R(const Array<float>& ke, Array<float>& t, Array<float>& R);

Array<float> assemble_homogeneous_3x4(const Array<float>& R, const Array<float>& t);

template <class TData>
inline FixedArray<TData, 4, 4> assemble_homogeneous_4x4(
    const FixedArray<TData, 3, 3>& R,
    const FixedArray<TData, 3>& t)
{
    return FixedArray<TData, 4, 4>{
            R(0, 0), R(0, 1), R(0, 2), t(0),
            R(1, 0), R(1, 1), R(1, 2), t(1),
            R(2, 0), R(2, 1), R(2, 2), t(2),
            0, 0, 0, 1};
}

Array<float> assemble_inverse_homogeneous_3x4(const Array<float>& R, const Array<float>& t);

template <class TData>
inline FixedArray<TData, 4, 4> assemble_inverse_homogeneous_4x4(
    const FixedArray<TData, 3, 3>& R,
    const FixedArray<TData, 3>& t)
{
    FixedArray<TData, 3, 3> Ri;
    FixedArray<TData, 3> ti;
    invert_t_R(t, R, ti, Ri);
    return assemble_homogeneous_4x4(Ri, ti);
}

Array<float> homogenized_4x4(const Array<float>& a);

Array<float> homogenized_4(const Array<float>& a);

inline FixedArray<float, 4> homogenized_4(const FixedArray<float, 3>& a) {
    return FixedArray<float, 4>{a(0), a(1), a(2), 1};
}

Array<float> homogenized_3(const Array<float>& a);

template <class TData>
inline FixedArray<TData, 3> homogenized_3(const FixedArray<TData, 2>& a)
{
    return FixedArray<TData, 3>{a(0), a(1), 1};
}

Array<float> homogenized_Nx3(const Array<float>& a);

Array<float> homogenized_Nx4(const Array<float>& a, float value = 1);

Array<float> dehomogenized_Nx3(const Array<float>& a, float value = 1);

Array<float> dehomogenized_Nx2(const Array<float>& a, float value = 1);

Array<float> dehomogenized_2(const Array<float>& a, float value = 1);

template <class TData>
inline FixedArray<TData, 2> dehomogenized_2(
    const FixedArray<TData, 3>& a,
    TData value = 1)
{
    assert(std::abs(a(2) - value) < 1e-12);
    return FixedArray<TData, 2>{a(0), a(1)};
}

Array<float> dehomogenized_3(const Array<float>& a);

inline FixedArray<float, 3> dehomogenized_3(const FixedArray<float, 4>& a) {
    assert(std::abs(a(3) - 1) < 1e-12);
    return FixedArray<float, 3>{a(0), a(1), a(2)};
}

Array<float> dehomogenized_3x4(const Array<float>& a);

Array<float> homogeneous_jacobian_dx(const Array<float>& M, const Array<float>& x);

}
