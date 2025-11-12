#pragma once
#include <vector>
#include <queue>
#include <climits>
#include <algorithm>

struct Edge {
    int next;
    int backward_edge;
    double cap;
    Edge(int next_node, int back_index, double capacity)
        : next(next_node), backward_edge(back_index), cap(capacity) {}
};

class Dinic {
public:
    int n;
    std::vector<std::vector<Edge>> adj;
    std::vector<int> level, start;

    Dinic(int n);

    void add_edge(int u, int v, double cap);
    // add_tedge: add terminal edges for a pixel node in the graph
    // Convention: Dinic graph size should be N = W*H + 2, where
    // source index == N-2 and sink index == N-1. add_tedge(v, bg, fg)
    // will add an edge source->v with capacity = fg and an edge v->sink
    // with capacity = bg (so DpFG is link to source, DpBG is link to sink).
    void add_tedge(int v, double bg, double fg);
    bool bfs(int s, int t);
    double dfs(int u, int t, double flow);
    double max_flow(int s, int t);

    // --- new additions for integration ---
    std::vector<bool> minCut(int s); // get reachable nodes after maxflow
};
