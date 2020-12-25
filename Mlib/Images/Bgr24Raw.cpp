#include "Bgr24Raw.hpp"
#include <Mlib/String.hpp>
#include <fstream>
#include <regex>

using namespace Mlib;

Bgr24Raw Bgr24Raw::load_from_file(const std::string& filename) {
    static const std::regex re("^.*-(\\d+)x(\\d+)x(\\d+)\\.bgr$");
    std::smatch match;
    if (std::regex_match(filename, match, re)) {
        if (match[3] != "24") {
            throw std::runtime_error("Only 24-bit raw images are supported");
        }
        std::ifstream istream(filename, std::ios_base::binary);
        try {
            // size is given in width*height, but we deal with rows*columns
            // => swap match 1 and 2
            return load_from_stream(
                ArrayShape{
                    static_cast<size_t>(safe_stoi(match[2].str())),
                    static_cast<size_t>(safe_stoi(match[1].str()))},
                istream);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(e.what() + std::string(": ") + filename);
        }
    } else {
        throw std::runtime_error("Filename "+ filename + " does not match ^.*-(\\d+)x(\\d+)x(\\d+)\\.bgr$");
    }
}

Bgr24Raw Bgr24Raw::load_from_stream(const ArrayShape& shape, std::istream& istream) {
    Bgr24Raw result;
    result.do_resize(shape);
    istream.read(reinterpret_cast<char*>(&result(0, 0)), result.nbytes());
    if (istream.fail()) {
        throw std::runtime_error("Could not read raw image");
    }
    return result;
}

Array<float> Bgr24Raw::to_float_grayscale() const {
    Array<float> grayscale(shape());
    Array<Bgr24> f = flattened();
    Array<float> g = grayscale.flattened();
    for (size_t i = 0; i < g.length(); i++) {
        g(i) = (
            static_cast<float>(f(i).r) / 255 +
            static_cast<float>(f(i).g) / 255 +
            static_cast<float>(f(i).b) / 255) / 3;
        assert(g(i) >= 0);
        assert(g(i) <= 1);
    }
    return grayscale;
}

Array<float> Bgr24Raw::to_float_rgb() const {
    Array<float> result(ArrayShape{3}.concatenated(shape()));
    Array<float> R = result[0];
    Array<float> G = result[1];
    Array<float> B = result[2];
    for (size_t r = 0; r < shape(0); ++r) {
        for (size_t c = 0; c < shape(1); ++c) {
            R(r, c) = static_cast<float>((*this)(r, c).r) / 255;
            G(r, c) = static_cast<float>((*this)(r, c).g) / 255;
            B(r, c) = static_cast<float>((*this)(r, c).b) / 255;
        }
    }
    return result;
}
