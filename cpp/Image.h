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

    //Alternative approach but we have not used it here
    Image(const std::vector<uint8_t>& raw, int width, int height, int channels = 3);

    [[nodiscard]] constexpr int width() const noexcept { return W; }
    [[nodiscard]] constexpr int height() const noexcept { return H; }
    [[nodiscard]] constexpr int channels() const noexcept { return C; }

    //Get colour at pixel indexed at (x, y)
    /*
    We will be using this short function many times in our logic (millions of times)
    Inline execution eliminates call overhead
    */
    [[nodiscard]] inline Vec3 getColor(int x, int y) const noexcept {

        //we use a single dimensional array and access pixel (x, y) using index = y * W + x
        const size_t idx = (static_cast<size_t>(y) * W + x) * C;
        return Vec3{
            static_cast<double>(data[idx]),
            static_cast<double>(data[idx + 1]),
            static_cast<double>(data[idx + 2])
        };
    }

    //return the whole table (if needed)
    [[nodiscard]] const std::vector<uint8_t>& raw() const noexcept { return data; }

private:
    int W, H, C;
    
    // Aligned memory for potential SIMD operations
    alignas(32) std::vector<uint8_t> data;
};
