#include "GraphBuilder.h"
#include <cmath>

GraphBuilder::GraphBuilder(const Image& img, const DataModel& dm, double lambda_)
    : image(img), dataModel(dm), W(img.width()), H(img.height()), lambda(lambda_) {}

double GraphBuilder::computeBeta(const Image& img) {
    int W = img.width(), H = img.height();
    double sum = 0.0;
    long long cnt = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            Vec3 c = img.getColor(x, y);
            if (x + 1 < W) {
                Vec3 c2 = img.getColor(x+1, y);
                double d = (c.r - c2.r)*(c.r - c2.r) + (c.g - c2.g)*(c.g - c2.g) + (c.b - c2.b)*(c.b - c2.b);
                sum += d; ++cnt;
            }
            if (y + 1 < H) {
                Vec3 c2 = img.getColor(x, y+1);
                double d = (c.r - c2.r)*(c.r - c2.r) + (c.g - c2.g)*(c.g - c2.g) + (c.b - c2.b)*(c.b - c2.b);
                sum += d; ++cnt;
            }
        }
    }
    double mean = cnt > 0 ? (sum / cnt) : 1.0;
    double beta = 1.0 / (2.0 * mean + 1e-9);
    return beta;
}

std::unique_ptr<Dinic> GraphBuilder::buildGraph() {
    int nodes = W * H;
    int source = nodes;
    int sink = nodes + 1;
    std::unique_ptr<Dinic> G(new Dinic(nodes + 2));

    double beta = computeBeta(image);

    // add t-links
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int idx = y * W + x;
            double capS = dataModel.getDpBG(x, y); // source -> node (bg cost)
            double capT = dataModel.getDpFG(x, y); // node -> sink (fg cost)
            // add source->node with capS
            G->add_edge(source, idx, capS);
            // add node->sink with capT
            G->add_edge(idx, sink, capT);
        }
    }

    // add n-links (4-neighborhood)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int u = y * W + x;
            Vec3 cu = image.getColor(x, y);
            if (x + 1 < W) {
                int v = y * W + (x+1);
                Vec3 cv = image.getColor(x+1, y);
                double diff = (cu.r - cv.r)*(cu.r - cv.r) + (cu.g - cv.g)*(cu.g - cv.g) + (cu.b - cv.b)*(cu.b - cv.b);
                double w = lambda * std::exp(-beta * diff);
                // undirected edge implemented by adding two directed edges with same capacity
                G->add_edge(u, v, w);
                G->add_edge(v, u, w);
            }
            if (y + 1 < H) {
                int v = (y+1) * W + x;
                Vec3 cv = image.getColor(x, y+1);
                double diff = (cu.r - cv.r)*(cu.r - cv.r) + (cu.g - cv.g)*(cu.g - cv.g) + (cu.b - cv.b)*(cu.b - cv.b);
                double w = lambda * std::exp(-beta * diff);
                G->add_edge(u, v, w);
                G->add_edge(v, u, w);
            }
        }
    }

    return G;
}
