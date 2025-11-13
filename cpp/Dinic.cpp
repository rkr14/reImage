#include "Dinic.h"
#include <queue>
#include <algorithm>
#include <limits>

Dinic::Dinic(int n_) : n(n_), adj(n_), level(n_), it(n_) {}

void Dinic::add_edge(int u, int v, double cap) {
    Edge a(v, static_cast<int>(adj[v].size()), cap);
    Edge b(u, static_cast<int>(adj[u].size()), 0.0);
    adj[u].push_back(a);
    adj[v].push_back(b);
}

bool Dinic::bfs(int s, int t) {
    std::fill(level.begin(), level.end(), -1);
    std::queue<int> q;
    level[s] = 0;
    q.push(s);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (const Edge &e : adj[u]) {
            if (e.cap > 1e-12 && level[e.next] == -1) {
                level[e.next] = level[u] + 1;
                q.push(e.next);
            }
        }
    }
    return level[t] != -1;
}

double Dinic::dfs(int u, int t, double pushed) {
    if (u == t) return pushed;
    for (int &i = it[u]; i < (int)adj[u].size(); ++i) {
        Edge &e = adj[u][i];
        if (e.cap > 1e-12 && level[e.next] == level[u] + 1) {
            double tr = dfs(e.next, t, std::min(pushed, e.cap));
            if (tr > 0) {
                e.cap -= tr;
                adj[e.next][e.back].cap += tr;
                return tr;
            }
        }
    }
    return 0.0;
}

double Dinic::max_flow(int s, int t) {
    double flow = 0.0;
    const double INF = std::numeric_limits<double>::infinity();
    while (bfs(s, t)) {
        std::fill(it.begin(), it.end(), 0);
        while (true) {
            double pushed = dfs(s, t, INF);
            if (pushed <= 0) break;
            flow += pushed;
        }
    }
    return flow;
}

std::vector<bool> Dinic::minCut(int s) const {
    std::vector<bool> seen(n, false);
    std::vector<int> stack;
    stack.reserve(n);
    stack.push_back(s);
    seen[s] = true;
    while (!stack.empty()) {
        int u = stack.back(); stack.pop_back();
        for (const Edge &e : adj[u]) {
            if (e.cap > 1e-12 && !seen[e.next]) {
                seen[e.next] = true;
                stack.push_back(e.next);
            }
        }
    }
    return seen;
}
