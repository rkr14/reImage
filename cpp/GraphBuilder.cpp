#include "GraphBuilder.h"
#include "SimdOps.h"
#include <cmath>

GraphBuilder::GraphBuilder(const Image& img, const DataModel& dm, double lambda_)
    : image(img), dataModel(dm), W(img.width()), H(img.height()), lambda(lambda_) {}

double GraphBuilder::computeBeta(const Image& img) {
    const int W = img.width(), H = img.height();
    double sum = 0.0;
    long long cnt = 0;

#ifdef __AVX2__
    // AVX2 
    for (int y = 0; y < H; ++y) {
        int x = 0;
        // Process 4 pixels at a time
        for (; x + 4 < W; x += 4) {
            alignas(32) double colors_a[12], colors_b[12], dists[4];
            
            // Load 4 adjacent color pairs
            for (int i = 0; i < 4; ++i) {
                Vec3 c1 = img.getColor(x + i, y);
                Vec3 c2 = img.getColor(x + i + 1, y);
                colors_a[i*3] = c1.r; colors_a[i*3+1] = c1.g; colors_a[i*3+2] = c1.b;
                colors_b[i*3] = c2.r; colors_b[i*3+1] = c2.g; colors_b[i*3+2] = c2.b;
            }
            
            simd::colorDistSq4_AVX2(colors_a, colors_b, dists);
            sum += dists[0] + dists[1] + dists[2] + dists[3];
            cnt += 4;
        }
        
        // Handle remaining pixels
        for (; x + 1 < W; ++x) {
            Vec3 c1 = img.getColor(x, y);
            Vec3 c2 = img.getColor(x + 1, y);
            sum += simd::colorDistSq(c1, c2);
            ++cnt;
        }
    }
    
   
    for (int y = 0; y + 1 < H; ++y) {
        for (int x = 0; x < W; ++x) {
            Vec3 c1 = img.getColor(x, y);
            Vec3 c2 = img.getColor(x, y + 1);
            sum += simd::colorDistSq(c1, c2);
            ++cnt;
        }
    }
#else
    // Scalar 
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            Vec3 c = img.getColor(x, y);
            if (x + 1 < W) {
                Vec3 c2 = img.getColor(x+1, y);
                sum += simd::colorDistSq(c, c2);
                ++cnt;
            }
            if (y + 1 < H) {
                Vec3 c2 = img.getColor(x, y+1);
                sum += simd::colorDistSq(c, c2);
                ++cnt;
            }
        }
    }
#endif
    
    const double mean = (cnt > 0) ? (sum / cnt) : 1.0;
    const double beta = 1.0 / (2.0 * mean + 1e-9);
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

    // add n-links (4-neighborhood) - optimized for cache locality
    const double neg_beta = -beta;
    
    
    for (int y = 0; y < H; ++y) {
        const int row_offset = y * W;
        for (int x = 0; x + 1 < W; ++x) {
            const int u = row_offset + x;
            const int v = row_offset + x + 1;
            
            const Vec3 cu = image.getColor(x, y);
            const Vec3 cv = image.getColor(x + 1, y);
            const double diff = simd::colorDistSq(cu, cv);
            const double w = lambda * std::exp(neg_beta * diff);
            
            // Undirected edge
            G->add_edge(u, v, w);
            G->add_edge(v, u, w);
        }
    }
    
    // Vertical edges
    for (int y = 0; y + 1 < H; ++y) {
        const int row_offset = y * W;
        const int next_row = row_offset + W;
        for (int x = 0; x < W; ++x) {
            const int u = row_offset + x;
            const int v = next_row + x;
            
            const Vec3 cu = image.getColor(x, y);
            const Vec3 cv = image.getColor(x, y + 1);
            const double diff = simd::colorDistSq(cu, cv);
            const double w = lambda * std::exp(neg_beta * diff);
            
            G->add_edge(u, v, w);
            G->add_edge(v, u, w);
        }
    }

    return G;
}
