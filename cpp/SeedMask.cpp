#include "SeedMask.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>

SeedMask::SeedMask(const std::string& seed_bin_path, int width, int height)
    : W(width), H(height)
{
    size_t expected = static_cast<size_t>(W) * H;
    data.resize(expected);
    std::ifstream in(seed_bin_path, std::ios::binary);
    if (!in) throw std::runtime_error("SeedMask: failed to open " + seed_bin_path);
    in.read(reinterpret_cast<char*>(data.data()), expected);
    if (!in) throw std::runtime_error("SeedMask: failed to read expected bytes from " + seed_bin_path);
}

SeedMask::SeedMask(int width, int height, int x0, int y0, int x1, int y1)
    : W(width), H(height)
{
    data.assign(static_cast<size_t>(W)*H, -1);
    // clamp
    x0 = std::max(0, std::min(x0, W-1));
    x1 = std::max(0, std::min(x1, W-1));
    y0 = std::max(0, std::min(y0, H-1));
    y1 = std::max(0, std::min(y1, H-1));
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);

    // outside rectangle => sure background (0)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (x < x0 || x > x1 || y < y0 || y > y1) data[y * W + x] = 0;
            else data[y * W + x] = -1; // inside rectangle = unknown
        }
    }
}

int SeedMask::getLabel(int x, int y) const {
    if (x < 0 || x >= W || y < 0 || y >= H) return 0;
    return static_cast<int>(data[y * W + x]);
}
