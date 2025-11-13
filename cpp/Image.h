#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

struct Vec3 {
    double r, g, b;
};

class Image {
public:
    // Load from raw binary file written by Python: uint8 RGB interleaved, row-major
    Image(const std::string& path, int width, int height, int channels = 3);

    // Alternative: construct from raw vector (not used here, but provided)
    Image(const std::vector<uint8_t>& raw, int width, int height, int channels = 3);

    int width() const { return W; }
    int height() const { return H; }
    int channels() const { return C; }

    // Returns color in RGB order (0..255 each) as doubles
    Vec3 getColor(int x, int y) const;

    const std::vector<uint8_t>& raw() const { return data; }

private:
    int W, H, C;
    std::vector<uint8_t> data;
};
