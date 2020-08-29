#include "Epilines.hpp"
#include <Mlib/Geometry/Homogeneous.hpp>
#include <Mlib/Images/Coordinates.hpp>
#include <Mlib/Images/PpmImage.hpp>
#include <Mlib/Math/Approximate_Rank.hpp>
#include <Mlib/Sfm/Disparity/Epiline_Direction.hpp>
#include <Mlib/Sfm/Disparity/Inverse_Epiline_Direction.hpp>
#include <Mlib/Sfm/Rigid_Motion/Fundamental_Matrix.hpp>

using namespace Mlib;

void Mlib::Sfm::draw_epilines_from_epipole(
    const Array<float>& epipole,
    PpmImage& bmp,
    const Rgb24& color)
{
    assert(all(epipole.shape() == ArrayShape{2}));

    for(size_t r = 0; r < bmp.shape(0); r+=20) {
        for(size_t c = 0; c < bmp.shape(1); c+=20) {
            // Array<float> n = (F, i2a(ArrayShape{r, c}));
            bmp.draw_infinite_line(
                Array<float>{r + 0.5f, c + 0.5f},
                Array<float>{epipole(id1), epipole(id0)},
                0,
                color);
        }
    }
}

void Mlib::Sfm::draw_epilines_from_F(
    const Array<float>& F,
    PpmImage& bmp,
    const Rgb24& color,
    size_t spacing)
{
    assert(all(F.shape() == ArrayShape{3, 3}));

    for(size_t r = 0; r < bmp.shape(0); r += spacing) {
        for(size_t c = 0; c < bmp.shape(1); c += spacing) {
            EpilineDirection ed{r, c, F};
            if (!ed.good) {
                continue;
            }
            for(float s = -1; s <= 1; s += 2) {
                bmp.draw_infinite_line(
                    a2fi(ed.center1),
                    a2fi(ed.center1 + s * ed.v1),
                    0,
                    color);
                // bmp.draw_fill_rect(
                //     a2i(p),
                //     5,
                //     Bgr565::green());
            }
        }
    }
}

void Mlib::Sfm::draw_inverse_epilines_from_F(
    const Array<float>& F,
    PpmImage& bmp,
    const Rgb24& color,
    size_t spacing)
{
    assert(all(F.shape() == ArrayShape{3, 3}));

    for(size_t r = 0; r < bmp.shape(0); r += spacing) {
        for(size_t c = 0; c < bmp.shape(1); c += spacing) {
            InverseEpilineDirection ied{r, c, F};
            if (!ied.good) {
                continue;
            }
            for(float s = -1; s <= 1; s += 2) {
                bmp.draw_infinite_line(
                    a2fi(ied.center0),
                    a2fi(ied.center0 + s * ied.v0),
                    0,
                    color);
            }
        }
    }
}
