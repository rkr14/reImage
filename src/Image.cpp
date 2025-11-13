class Image {
private:
    // Use aligned memory for SIMD operations
    alignas(32) std::vector<uint8_t> data;
    
public:
    // Inline hot path
    [[nodiscard]] inline Vec3 getColor(int x, int y) const noexcept {
        const size_t idx = (static_cast<size_t>(y) * W + x) * C;
        return Vec3{
            static_cast<double>(data[idx]),
            static_cast<double>(data[idx + 1]),
            static_cast<double>(data[idx + 2])
        };
    }
};