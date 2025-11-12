#pragma once
#include "DataModel.h"
#include "Image.h"
#include "../Segmentation/Segmentation/Dinic.h"  // Dinic header

class GraphBuilder {
public:
    GraphBuilder(const DataModel& data, const Image& img, double lambda, double beta);
    void buildGraph();
    Dinic& graph();

private:
    const DataModel& model;
    const Image& image;
    double lambda, beta;
    int W, H;
    Dinic dinic;
};
