#pragma once
#include <vector>
#include <queue>
#include <algorithm>
#include <limits>

/* each edge we form in graph we will store it's capacity to later check for flow and 'next' node
   is the direction it is originally and backward edge is formed as if we send less amount of
   flow then the capacity. It virtually acts as the left over capacity is flowing in the reverse direction*/
struct Edge {
    int next;
    int backward_edge;
    double cap;

    // constructor
    Edge(int next_node = 0, int back_index = 0, double capacity = 0.0)
        : next(next_node), backward_edge(back_index), cap(capacity) {}
};

/* All helper functions for Dinic's algorithm will be defined in this class.
   Dinic's algorithm finds maximum flow by repeatedly:
   1. Building a level graph via BFS
   2. Finding blocking flows via DFS */
class Dinic {
public:
    int n;
    // adjacency list representation of the flow network
    std::vector<std::vector<Edge>> adj;

    Dinic(int n = 0);
    void add_edge(int u, int v, double cap);
    bool bfs(int s, int t);
    double dfs(int u, int t, double pushed);
    double max_flow(int s, int t);
    std::vector<bool> minCut(int s) const;

private:
    /* We track each node's level during BFS from the source.
       This forms the layered residual graph used in Dinic's algorithm. */
    std::vector<int> level;

    /* Start array for DFS iteration.
       Keeps track of the current position in each node's adjacency list.
       This ensures DFS doesn't keep trying the same saturated edges repeatedly.
       Memory address of start[u] is given and increased every iteration,
       ensuring efficient exploration and avoiding re-visiting saturated edges. */
    std::vector<int> start;
};
