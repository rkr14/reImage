#pragma once
#include "DataModel.h"
#include "Image.h"
#include "Dinic.h"
#include <memory>

class GraphBuilder {
public:
    GraphBuilder(const Image& img, const DataModel& dm, double lambda = 50.0);

    // builds Dinic graph and returns owned pointer to it
    // nodes: 0 .. (W*H-1), source = W*H, sink = W*H+1
    std::unique_ptr<Dinic> buildGraph();

    static double computeBeta(const Image& img);

private:
    const Image& image;
    const DataModel& dataModel;
    int W, H;
    double lambda;
};
