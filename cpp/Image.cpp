#include "Image.h"
#include <fstream>
#include <iostream>

Image::Image(const std::string& path, int width, int height, int channels)
    : W(width), H(height), C(channels)
{
    const size_t expected = static_cast<size_t>(W) * H * C;
    data.reserve(expected);  // Reserve before resize for efficiency
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


