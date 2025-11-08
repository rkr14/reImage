#include "SeedMask.h"

SeedMask::SeedMask(const std::string& filePath, int width, int height)
    : W(width), H(height)
{
    std::ifstream in(filePath, std::ios::binary);
    if (!in) throw std::runtime_error("Failed to open " + filePath);
    data.resize(static_cast<size_t>(W) * H);
    in.read(reinterpret_cast<char*>(data.data()), data.size());
    if (!in) throw std::runtime_error("Incomplete seed file read: " + filePath);
}

int SeedMask::getLabel(int x, int y) const {
    return static_cast<int>(data[y * W + x]);
}
