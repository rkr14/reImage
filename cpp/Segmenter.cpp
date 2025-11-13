#include "Segmenter.h"
#include "MinCut.h"
#include <iostream>

void Segmenter::run(Dinic& G, int W, int H, int source, int sink, const std::string& outMaskPath) {
    std::cout << "Running maxflow..." << std::endl;
    double flow = G.max_flow(source, sink);
    std::cout << "Maxflow result: " << flow << std::endl;

    // get reachable set (nodes reachable from source in residual graph)
    std::vector<bool> reachable = G.minCut(source);
    // reachable has size G.n (including source and sink). We only need first W*H
    if ((int)reachable.size() < W*H) throw std::runtime_error("Segmenter: minCut size mismatch");

    MinCut::writeMaskToFile(reachable, W, H, outMaskPath);
    std::cout << "Wrote mask to " << outMaskPath << std::endl;
}
