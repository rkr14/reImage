#include "Image.h"
#include "SeedMask.h"
#include "DataModel.h"
#include "GraphBuilder.h"
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>
#include <cctype>

// tiny helper to extract integer fields "width" and "height" from a small JSON file
static bool read_meta_wh(const std::string& path, int& W, int& H) {
    std::ifstream ifs(path);
    if (!ifs) return false;
    std::string s((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    auto find_int = [&](const std::string& key, int& out)->bool{
        size_t p = s.find('"' + key + '"');
        if (p == std::string::npos) return false;
        p = s.find(':', p);
        if (p == std::string::npos) return false;
        ++p;
        // skip spaces
        while (p < s.size() && isspace((unsigned char)s[p])) ++p;
        // read number
        size_t q = p;
        while (q < s.size() && (s[q]=='-' || isdigit((unsigned char)s[q]))) ++q;
        if (q==p) return false;
        try {
            out = std::stoi(s.substr(p, q-p));
        } catch(...) { return false; }
        return true;
    };
    return find_int("width", W) && find_int("height", H);
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: ./segment image.bin meta.json seed.bin\n";
        return 1;
    }

    std::string imgPath = argv[1];
    std::string metaPath = argv[2];
    std::string seedPath = argv[3];

    // Parse meta.json (tiny parser used above)
    int W = 0, H = 0;
    if (!read_meta_wh(metaPath, W, H)) {
        std::cerr << "Failed to read meta file (width/height) " << metaPath << "\n";
        return 1;
    }

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
    GraphBuilder builder(model, img, lambda, beta);
    builder.buildGraph();

    std::cout << "Graph built. Ready for maxflow.\n";

    // Run max-flow
    double flow = builder.graph().max_flow(W*H, W*H+1);
    std::cout << "Max flow: " << flow << "\n";
        // extract min-cut (reachable from source -> foreground)
        int source = W * H;
        auto reachable = builder.graph().minCut(source);

        // write mask output: either argv[4] or metaPath + ".mask.bin"
        std::string maskOut;
        if (argc >= 5) maskOut = argv[4];
        else maskOut = metaPath + ".mask.bin";

        std::ofstream mf(maskOut, std::ios::binary);
        if (!mf) {
            std::cerr << "Failed to open mask output file: " << maskOut << "\n";
            return 1;
        }
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int idx = y * W + x;
                uint8_t v = reachable[idx] ? 1 : 0;
                mf.write(reinterpret_cast<const char*>(&v), 1);
            }
        }
        mf.close();

        std::cout << "Wrote mask to " << maskOut << "\n";

        return 0;
}
