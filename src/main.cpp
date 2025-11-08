#include "Image.h"
#include "SeedMask.h"
#include "DataModel.h"
#include "GraphBuilder.h"
#include "utils/json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: ./segment image.bin meta.json seed.bin\n";
        return 1;
    }

    std::string imgPath = argv[1];
    std::string metaPath = argv[2];
    std::string seedPath = argv[3];

    // Parse meta.json
    std::ifstream jfile(metaPath);
    json meta;
    jfile >> meta;
    int W = meta["width"];
    int H = meta["height"];

    // Load data
    Image img(imgPath, W, H);
    SeedMask seeds(seedPath, W, H);

    // Parameters
    DataModel model(8, 1.0, 1e-9);
    model.buildHistograms(img, seeds);
    model.computeDataCosts(img, seeds);

    // Build graph
    double lambda = 50.0;
    double beta = 0.1;
    GraphBuilder builder(model, lambda, beta);
    builder.buildGraph();

    std::cout << "Graph built. Ready for maxflow.\n";

    // You’ll call builder.graph().maxflow() here once Dinic is ready
    return 0;
}
