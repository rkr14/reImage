#pragma once
#include "DataModel.h"
#include "../DINIC/dinic.cpp"  // your custom maxflow implementation

class GraphBuilder {
public:
    GraphBuilder(const DataModel& data, double lambda, double beta);
    void buildGraph();
    Dinic& graph();

private:
    const DataModel& model;
    double lambda, beta;
    Dinic dinic;
    int W, H;
};
