#include "GraphBuilder.h"
#include <cmath>

GraphBuilder::GraphBuilder(const DataModel& data, double lambda_, double beta_)
    : model(data), lambda(lambda_), beta(beta_),
      W(data.width()), H(data.height()),
      dinic(W * H + 2) {}

void GraphBuilder::buildGraph() {
    int source = W * H;
    int sink = W * H + 1;

    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            int idx = y * W + x;
            dinic.add_tedge(idx, model.getDpBG(x, y), model.getDpFG(x, y));
        }

    // Add n-links (4-neighbourhood)
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            Vec3 c = model.getColor(x, y);
            auto addEdge = [&](int nx, int ny) {
                if (nx < 0 || nx >= W || ny < 0 || ny >= H) return;
                Vec3 nc = model.getColor(nx, ny);
                double diff = (c.r - nc.r)*(c.r - nc.r) + (c.g - nc.g)*(c.g - nc.g) + (c.b - nc.b)*(c.b - nc.b);
                double cap = lambda * std::exp(-beta * diff);
                int u = y * W + x, v = ny * W + nx;
                dinic.add_edge(u, v, cap);
                dinic.add_edge(v, u, cap);
            };
            addEdge(x + 1, y);
            addEdge(x, y + 1);
        }
}
Dinic& GraphBuilder::graph() { return dinic; }
