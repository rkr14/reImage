#pragma once
#include <vector>

struct Edge {
    int next;
    int back;   // index of reverse edge in next node's adj list
    double cap;
    Edge(int n = 0, int b = 0, double c = 0.0) : next(n), back(b), cap(c) {}
};

class Dinic {
public:
    Dinic(int n = 0);

    // graph operations
    void add_edge(int u, int v, double cap);

    // maxflow
    double max_flow(int s, int t);

    // after max_flow, get which nodes are reachable from s in residual graph
    std::vector<bool> minCut(int s) const;

    // public members for introspection (used by MinCut helper)
    int n;
    std::vector<std::vector<Edge>> adj;

private:
    std::vector<int> level;
    std::vector<int> it; // iterator for DFS

    bool bfs(int s, int t);
    double dfs(int u, int t, double pushed);
};
