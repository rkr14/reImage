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
    Image(const std::string& filePath, int width, int height, int channels = 3);

    int width() const { return W; }
    int height() const { return H; }

    Vec3 getColor(int x, int y) const;

private:
    int W, H, C;
    std::vector<uint8_t> data;
};
