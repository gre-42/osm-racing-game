#include "stb_array.h"
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "stb_mipmaps.h"
#include <Mlib/Floating_Point_Exceptions.hpp>
#include <Mlib/Images/Filters/Gaussian_Filter.hpp>
#include <algorithm>
#include <memory>
#include <stdexcept>

using namespace Mlib;

void downsample_rgba_inplace(
    unsigned char* data,
    unsigned char* downsampled_data,
    int width,
    int height)
{
    Array<float> ar = stb_image_2_array(data, width, height, 4).casted<float>();
    Array<float> m =
        gaussian_filter_NWE(ar[3], 1.f, float{NAN})
        .applied([](float v){return v == 0 ? float{NAN} : v;});
    for (int d = 0; d < 3; ++d) {
        TemporarilyIgnoreFloatingPointExeptions ignore_except;
        ar[d] = gaussian_filter_NWE(ar[d] * ar[3], 1.f, float{NAN}) / m;
    }
    array_2_stb_image(substitute_nans(ar, 0.f).casted<unsigned char>(), data);
    {
        TemporarilyIgnoreFloatingPointExeptions ignore_except;
        if (!stbir_resize_uint8(
            data,
            width,
            height,
            0,
            downsampled_data,
            (width == 0 || height == 0) ? 1 : width / 2,
            (width == 0 || height == 0) ? 1 : height / 2,
            0,
            4))
        {
            throw std::runtime_error("could not resize image");
        }
    }
}

void downsample_rgba_inplace0(
    unsigned char* data,
    unsigned char* downsampled_data,
    int width,
    int height)
{
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            int i = (r * width + c) * 4;
            for (int d = 0; d < 3; ++d) {
                data[i + d] = (unsigned char)(data[i + d] * (float(data[i + 3]) / 255));
            }
        }
    }
    {
        TemporarilyIgnoreFloatingPointExeptions ignore_except;
        if (!stbir_resize_uint8(
            data,
            width,
            height,
            0,
            downsampled_data,
            (width == 0 || height == 0) ? 1 : width / 2,
            (width == 0 || height == 0) ? 1 : height / 2,
            0,
            4))
        {
            throw std::runtime_error("could not resize image");
        }
    }
    for (int r = 0; r < height / 2; ++r) {
        for (int c = 0; c < width / 2; ++c) {
            int i = r * (width / 2) * 4 + c * 4;
            for (int d = 0; d < 3; ++d) {
                if (downsampled_data[i + 3] != 0) {
                    downsampled_data[i + d] = (unsigned char)std::clamp(downsampled_data[i + d] / (float(downsampled_data[i + 3]) / 255.f), 0.f, 255.f);
                } else {
                    downsampled_data[i + d] = ((d == 0) * 255);
                }
            }
        }
    }
}

RgbaDownsampler::RgbaDownsampler(unsigned char* data, int width, int height)
: data_{data},
  downsampled_data_{nullptr},
  width_{width},
  height_{height}
{
}
RgbaImage RgbaDownsampler::next() {
    if (downsampled_data_ == nullptr) {
        if ((width_ / 2 != 0) || (height_ / 2 != 0)) {
            buffer_.reset(new unsigned char[
                size_t(std::max(1, width_ / 2)) * size_t(std::max(1, height_ / 2)) * 4]);
            downsampled_data_ = buffer_.get();
        }
    } else {
        if ((std::max(1, width_) == 1 && std::max(1, height_) == 1)) {
            return RgbaImage{
                .data = nullptr,
                .width = 0,
                .height = 0};
        }
        downsample_rgba_inplace(
            data_,
            downsampled_data_,
            std::max(1, width_),
            std::max(1, height_));
        width_ /= 2;
        height_ /= 2;
        std::swap(data_, downsampled_data_);
    }
    return RgbaImage{
        .data = data_,
        .width = std::max(1, width_),
        .height = std::max(1, height_)};
}
