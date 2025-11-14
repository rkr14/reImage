#include "Dinic.h"

Dinic::Dinic(int n_) : n(n_), adj(n_), level(n_), start(n_) {}

/* For edge u->v, the actual edge is the edge 'a' as it is from u to v
   and with the capacity stated, while the edge 'b' is the reverse edge that
   we form with initially zero capacity. */
void Dinic::add_edge(int u, int v, double cap) {
    Edge a(v, static_cast<int>(adj[v].size()), cap);
    Edge b(u, static_cast<int>(adj[u].size()), 0.0);
    adj[u].push_back(a);
    adj[v].push_back(b);
}

/* s: Source, t: Sink
   Traverse from source and mark levels of each node from the source.
   This also ensures we only consider edges with positive capacity.
   Finally, the boolean of level[t] != -1 is returned which ensures that
   the path is reached from the source to sink or not. */
bool Dinic::bfs(int s, int t) {
    std::fill(level.begin(), level.end(), -1);
    std::queue<int> q;
    level[s] = 0;
    q.push(s);
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (const Edge &e : adj[u]) {
            if (e.cap > 1e-12 && level[e.next] == -1) {
                level[e.next] = level[u] + 1;
                q.push(e.next);
            }
        }
    }
    return level[t] != -1;
}

/* DFS reached the sink so we stop DFS and return the minimum flow of that path.
   
   Memory address of the start is given and then increased every iteration.
   It ensures that DFS doesn't keep getting stuck in the same edge or a node
   whose all outgoing edges are saturated.
   
   Only follows edges that go exactly one level deeper (level[next] == level[u] + 1).
   
   For each valid edge, recursively push flow. If successful, update the edge capacity
   and reverse edge capacity. Return the saturation flow pushed. */
double Dinic::dfs(int u, int t, double pushed) {
    // DFS reached the sink, return the flow we've pushed
    if (u == t) return pushed;

    // start[u] tracks current position in adjacency list of u
    for (int &i = start[u]; i < (int)adj[u].size(); ++i) {
        Edge &e = adj[u][i];
        // Only follow edges with positive capacity that go one level deeper
        if (e.cap > 1e-12 && level[e.next] == level[u] + 1) {
            double current_saturation = dfs(e.next, t, std::min(pushed, e.cap));
            if (current_saturation > 0) {
                // Update forward edge capacity
                e.cap -= current_saturation;
                // Update reverse edge capacity
                adj[e.next][e.backward_edge].cap += current_saturation;
                return current_saturation;
            }
        }
    }
    return 0.0;
}

/* BFS ensures that while there is a path from s to t, we build the level graph,
   find the paths via DFS, and traverse them while pushing flow.
   
   Multiple DFS calls push flow until no more can be sent in this level graph.
   Add the saturation flow of each path to get the max flow.
   
   Returns: maximum flow value from source to sink */
double Dinic::max_flow(int s, int t) {
    double flow = 0.0;
    const double INF = std::numeric_limits<double>::infinity();
    
    /* While there exists a path from s to t in the residual graph */
    while (bfs(s, t)) {
        // Reset start for every BFS phase
        std::fill(start.begin(), start.end(), 0);
        // Find all blocking flows in this level graph
        while (true) {
            double f = dfs(s, t, INF);
            if (f <= 0) break;
            flow += f;
        }
    }
    return flow;
}

/* After max_flow, get which nodes are reachable from source in residual graph.
   This determines the minimum cut by finding all nodes still reachable from source
   after all flows have been pushed. These reachable nodes form one side of the cut,
   and unreachable nodes form the other side.
   Uses iterative DFS with a stack to avoid recursion depth issues. */
std::vector<bool> Dinic::minCut(int s) const {
    std::vector<bool> seen(n, false);
    std::vector<int> stack;
    stack.reserve(n);
    stack.push_back(s);
    seen[s] = true;
    while (!stack.empty()) {
        int u = stack.back();
        stack.pop_back();
        for (const Edge &e : adj[u]) {
            if (e.cap > 1e-12 && !seen[e.next]) {
                seen[e.next] = true;
                stack.push_back(e.next);
            }
        }
    }
    return seen;
}
