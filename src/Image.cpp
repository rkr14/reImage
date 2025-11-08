#include "Image.h"
#include <fstream>

Image::Image(const std::string& filePath, int width, int height, int channels)
    : W(width), H(height), C(channels)
{
    std::ifstream in(filePath, std::ios::binary);
    if (!in) throw std::runtime_error("Failed to open " + filePath);
    data.resize(static_cast<size_t>(W) * H * C);
    in.read(reinterpret_cast<char*>(data.data()), data.size());
    if (!in) throw std::runtime_error("Incomplete image file read: " + filePath);
}

Vec3 Image::getColor(int x, int y) const {
    size_t idx = (static_cast<size_t>(y) * W + x) * C;
    return { double(data[idx]), double(data[idx + 1]), double(data[idx + 2]) };
}
