#include <iostream>
#include <string>
#include <memory>
#include <cstdlib>

#include "Image.h"
#include "SeedMask.h"
#include "DataModel.h"
#include "GraphBuilder.h"
#include "Segmenter.h"
#include "Dinic.h"

// Usage:
// 1) rectangle mode:
//    ./segment image.bin width height rect x0 y0 x1 y1 out_mask.bin
// 2) mask mode:
//    ./segment image.bin width height mask seed.bin out_mask.bin
//
// Example (rect):
//    ./segment data/cat.image.bin 640 480 rect 50 30 250 220 data/output_mask.bin
//
// Example (mask):
//    ./segment data/cat.image.bin 640 480 mask data/cat.seed.bin data/output_mask.bin

int main(int argc, char** argv) {
    if (argc < 7) {
        std::cerr << "Usage:\n  Rect mode: " << argv[0] << " image.bin W H rect x0 y0 x1 y1 out_mask.bin\n"
                  << "  Mask mode: " << argv[0] << " image.bin W H mask seed.bin out_mask.bin\n";
        return 1;
    }

    std::string imageBin = argv[1];
    int W = std::atoi(argv[2]);
    int H = std::atoi(argv[3]);
    std::string mode = argv[4];

    std::unique_ptr<SeedMask> seeds;
    std::string outMaskPath;

    try {
        if (mode == "rect") {
            if (argc < 10) {
                std::cerr << "Rect requires x0 y0 x1 y1 out_mask.bin\n";
                return 1;
            }
            int x0 = std::atoi(argv[5]);
            int y0 = std::atoi(argv[6]);
            int x1 = std::atoi(argv[7]);
            int y1 = std::atoi(argv[8]);
            outMaskPath = argv[9];
            seeds.reset(new SeedMask(W, H, x0, y0, x1, y1));
        } else if (mode == "mask") {
            if (argc < 7) {
                std::cerr << "Mask mode requires seed.bin and out_mask.bin\n";
                return 1;
            }
            std::string seedBin = argv[5];
            outMaskPath = argv[6];
            seeds.reset(new SeedMask(seedBin, W, H));
        } else {
            std::cerr << "Unknown seed mode: " << mode << "\n";
            return 1;
        }

        Image img(imageBin, W, H, 3);
        DataModel dm(8, 1.0, 1e-9);

        std::cout << "Building histograms..." << std::endl;
        dm.buildHistograms(img, *seeds);
        std::cout << "Computing data costs..." << std::endl;
        dm.computeDataCosts(img, *seeds);

        double lambda = 50.0;
        GraphBuilder gb(img, dm, lambda);
        auto Gptr = gb.buildGraph();
        int nodes = W * H;
        int source = nodes;
        int sink = nodes + 1;

        Segmenter::run(*Gptr, W, H, source, sink, outMaskPath);
    } catch (const std::exception &e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
