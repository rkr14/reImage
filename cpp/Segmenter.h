#pragma once
#include "Dinic.h"
#include "DataModel.h"
#include "Image.h"
#include <string>

class Segmenter {
public:
    // runs maxflow on given graph (Dinic) and writes output mask to outMaskPath (uint8 0/1 per pixel)
    static void run(Dinic& G, int W, int H, int source, int sink, const std::string& outMaskPath);
};
