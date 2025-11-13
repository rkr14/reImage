#pragma once
#include "Dinic.h"
#include <fstream>
#include <string>

// Helper utilities related to mincut (small, header-only)
struct MinCut {
    // write mask (uint8 0/1) to file in row-major order (expects reachable.size() == W*H)
    static void writeMaskToFile(const std::vector<bool>& reachable, int W, int H, const std::string& outPath) {
        std::ofstream out(outPath, std::ios::binary);
        if (!out) throw std::runtime_error("MinCut: failed to open output mask file");
        // write as uint8 values 0/1 per pixel
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                bool fg = reachable[y * W + x];
                uint8_t v = fg ? 1 : 0;
                out.write(reinterpret_cast<const char*>(&v), 1);
            }
        }
        out.close();
    }
};
