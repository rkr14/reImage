#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>

class SeedMask {
public:
    SeedMask(const std::string& filePath, int width, int height);
    int getLabel(int x, int y) const;

private:
    int W, H;
    std::vector<int8_t> data;
};
