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

    [[nodiscard]] constexpr int width() const noexcept { return W; }
    [[nodiscard]] constexpr int height() const noexcept { return H; }
    [[nodiscard]] constexpr int channels() const noexcept { return C; }

    // Inline hot path for performance - called millions of times during graph building
    [[nodiscard]] inline Vec3 getColor(int x, int y) const noexcept {
        const size_t idx = (static_cast<size_t>(y) * W + x) * C;
        return Vec3{
            static_cast<double>(data[idx]),
            static_cast<double>(data[idx + 1]),
            static_cast<double>(data[idx + 2])
        };
    }

    [[nodiscard]] const std::vector<uint8_t>& raw() const noexcept { return data; }

private:
    int W, H, C;
    // Aligned memory for potential SIMD operations
    alignas(32) std::vector<uint8_t> data;
};
