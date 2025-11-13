#include "Image.h"
#include <fstream>
#include <iostream>

Image::Image(const std::string& path, int width, int height, int channels)
    : W(width), H(height), C(channels)
{
    size_t expected = static_cast<size_t>(W) * H * C;
    data.resize(expected);
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Image: failed to open " + path);
    in.read(reinterpret_cast<char*>(data.data()), expected);
    if (!in) throw std::runtime_error("Image: failed to read expected bytes from " + path);
}

Image::Image(const std::vector<uint8_t>& raw, int width, int height, int channels)
    : W(width), H(height), C(channels), data(raw)
{
    if (data.size() != static_cast<size_t>(W) * H * C)
        throw std::runtime_error("Image: raw data size mismatch");
}

Vec3 Image::getColor(int x, int y) const {
    if (x < 0 || x >= W || y < 0 || y >= H) throw std::out_of_range("Image::getColor");
    size_t idx = (static_cast<size_t>(y) * W + x) * C;
    uint8_t R = data[idx + 0];
    uint8_t G = data[idx + 1];
    uint8_t B = data[idx + 2];
    return Vec3{ double(R), double(G), double(B) };
}
