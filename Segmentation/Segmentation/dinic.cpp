// #include <bits/stdc++.h>
// using namespace std;


// /* each edge we form in graph we will store it's capacity to later check for flow and 'next' node
//    is the direction it is originally and backward edge is formed as if we send less amount of
//    flow then the capacity. It virtually acts as the left over capacity is flowing in the revrse direction*/
// struct Edge {
//     int next;
//     int backward_edge;
//     long long cap;

//     //constructor
//     Edge(int next_node, int back_index, long long capacity){
//         next = next_node;
//         backward_edge = back_index;
//         cap = capacity;
// }

// };
// /*all helper functions for dinic will be defined in this class*/
// class Dinic {
// public:
//     int n;
//     //adjacency list
//     vector<vector<Edge>> adj;
//     /*we track each node’s level during BFS from the source 
//     this forms the layered residual graph used in Dinic’s algorithm.*/
//     vector<int> level;
//     vector<int> start;
//     Dinic(int n): n(n), adj(n), level(n), start(n){}

//     /*for edge u->v the actual edge is the edge a as it is from u to v
//     and with the capacity stated while the edge b is the reverse edge that
//     we formed with initially zero capacity*/
//     void add_edge(int u, int v, long long cap) {
//         Edge a = Edge(v,(int)adj[v].size(),cap);
//         Edge b = Edge(u,(int)adj[u].size(),0);
//         adj[u].push_back(a);
//         adj[v].push_back(b);
//     }
//     /* s:Source,t:Sink
//     traverse from source and mark levels of each node from the source
//     and it also ensures positive capacity and finally boolean of 
//     level[t]!=-1 is returned which ensures that the path is recahed from
//     the source to sink or not
//     */
//     bool bfs(int s, int t){
//         fill(level.begin(), level.end(), -1);
//         queue<int> q;
//         level[s] = 0;
//         q.push(s);
//         while(!q.empty()){
//             int u = q.front();
//             q.pop();
//             for (auto &e : adj[u]) {
//                 if(e.cap > 0 && level[e.next] == -1){
//                     level[e.next] = level[u] + 1;
//                     q.push(e.next);
//                 }
//             }
//         }
//         if(level[t] != -1) return true;
//         else return false;
//     }

//     long long dfs(int u, int t, long long flow){
//         /*DFS reached the sink so we stop DFS 
//         and return the minimuum flow of that path*/
//         if(u==t) return flow;

//         /*memory address of the start is given and then
//         increased every iteration it ensures that DFS
//         doesn't keep getting stuck in the same DFS or 
//         a node whose all outgoing edges are saturated*/
//         for(int &i = start[u];i<adj[u].size();i++){
//             Edge &e = adj[u][i];
//             //only follows the edges that go one level deeper
//             if(e.cap>0 and level[e.next]==level[u]+1){
//                 long long current_saturation = dfs(e.next,t,min(flow,e.cap));
//                 if(current_saturation>0){
//                     e.cap -= current_saturation;
//                     adj[e.next][e.backward_edge].cap += current_saturation;
//                     return current_saturation;
//                 }
//             }
//         }
//         return 0;
//     }
//     long long max_flow(int s,int t){
//         long long flow = 0;
//         /*BFS ensures that while there is a path
//         from s->t we build the level graph find the
//         paths traverse them and multiple DFS calls push
//         flow until no more can be sent in this level 
//         graph add the saturation flow
//         of each path ot get the max flow*/
//         while(bfs(s,t)){
//             //reset start for every BFS phase
//             fill(start.begin(), start.end(), 0);
//             while(true){
//                 long long f = dfs(s,t,LLONG_MAX);
//                 if(f==0) break;
//                 flow+=f;
//             }
//         }
//         return flow;
//     }

//     /*saving the residual graph to use in the min cut*/
//     void saveResidualGraph(const string &filename) {
//     ofstream fout(filename);
//     fout << n << "\n";
//     for (int u =0;u<n;u++) {
//         for (auto &e : adj[u]) {
//             fout << u << " " << e.next << " " << e.cap << "\n";
//         }
//     }
//     fout.close();
// }

// };



// int main() {
//     int n, m;
//     cin >>n>>m;
//     Dinic D(n);
//     for (int i = 0; i < m; i++) {
//         int u, v;
//         long long c;
//         cin >> u >> v >> c;
//         D.add_edge(u, v, c);
//     }
//     int s,t;
//     cin>>s>>t;
//     D.max_flow(s, t);
//     D.saveResidualGraph("residual_graph.txt");
// }


#include "Dinic.h"
#include <iostream>
#include <stack>

Dinic::Dinic(int n)
    : n(n), adj(n), level(n), start(n) {}
/* each edge we form in graph we will store it's capacity to later check for flow and 'next' node
is the direction it is originally and backward edge is formed as if we send less amount of
flow then the capacity. It virtually acts as the left over capacity is flowing in the revrse direction*/

/*for edge u->v the actual edge is the edge a as it is from u to v
and with the capacity stated while the edge b is the reverse edge that
we formed with initially zero capacity*/
void Dinic::add_edge(int u, int v, double cap) {
    Edge a(v, (int)adj[v].size(), cap);
    Edge b(u, (int)adj[u].size(), 0.0);
    adj[u].push_back(a);
    adj[v].push_back(b);
}

// Add terminal edges for pixel node v assuming last two nodes are source and sink
// We follow the convention: graph size = W*H + 2, source = n-2, sink = n-1.
// Parameters are passed as (v, bg, fg) so callers can pass (DpBG, DpFG).
void Dinic::add_tedge(int v, double bg, double fg) {
    int source = n - 2;
    int sink = n - 1;
    // Terminal edge mapping:
    // source -> v should carry the cost of assigning v to BACKGROUND (bg)
    // v -> sink should carry the cost of assigning v to FOREGROUND (fg)
    // This matches the usual graph-cut convention where cutting source->v
    // means assigning to background and cutting v->sink means assigning to foreground.
    add_edge(source, v, bg);
    add_edge(v, sink, fg);
}

/* s:Source,t:Sink
traverse from source and mark levels of each node from the source
and it also ensures positive capacity and finally boolean of 
level[t]!=-1 is returned which ensures that the path is recahed from
the source to sink or not
*/
bool Dinic::bfs(int s, int t) {
    std::fill(level.begin(), level.end(), -1);
    std::queue<int> q;
    level[s] = 0;
    q.push(s);
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (auto &e : adj[u]) {
            if (e.cap > 1e-9 && level[e.next] == -1) {
                level[e.next] = level[u] + 1;
                q.push(e.next);
            }
        }
    }
    return level[t] != -1;
}

double Dinic::dfs(int u, int t, double flow) {

    /*memory address of the start is given and then
    increased every iteration it ensures that DFS
    doesn't keep getting stuck in the same DFS or 
    a node whose all outgoing edges are saturated*/   

    if (u == t) return flow;

    for (int &i = start[u]; i < (int)adj[u].size(); ++i) {

        Edge &e = adj[u][i];
        if (e.cap > 1e-9 && level[e.next] == level[u] + 1) {

            double pushed = dfs(e.next, t, std::min(flow, e.cap));

            if (pushed > 1e-9) {
                e.cap -= pushed;
                adj[e.next][e.backward_edge].cap += pushed;
                return pushed;
            }
        }
    }
    return 0.0;
}

double Dinic::max_flow(int s, int t) {
    /*BFS ensures that while there is a path
    from s->t we build the level graph find the
    paths traverse them and multiple DFS calls push
    flow until no more can be sent in this level 
    graph add the saturation flow
    of each path ot get the max flow*/

    double flow = 0.0;
    const double INF = 1e18;
    while (bfs(s, t)) {
        std::fill(start.begin(), start.end(), 0);
        while (true) {
            double pushed = dfs(s, t, INF);
            if (pushed < 1e-9) break;
            flow += pushed;
        }
    }
    return flow;
}

std::vector<bool> Dinic::minCut(int s) {
    std::vector<bool> visited(n, false);
    std::stack<int> st;
    st.push(s);
    visited[s] = true;

    while (!st.empty()) {
        int u = st.top();
        st.pop();
        for (auto &e : adj[u]) {
            if (e.cap > 1e-9 && !visited[e.next]) {
                visited[e.next] = true;
                st.push(e.next);
            }
        }
    }
    return visited;
}

