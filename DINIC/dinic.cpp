#include <bits/stdc++.h>
using namespace std;


/* each edge we form in graph we will store it's capacity to later check for flow and 'next' node
   is the direction it is originally and backward edge is formed as if we send less amount of
   flow then the capacity. It virtually acts as the left over capacity is flowing in the revrse direction*/
struct Edge {
    int next;
    int backward_edge;
    long long cap;

    //constructor
    Edge(int next_node, int back_index, long long capacity){
        next = next_node;
        backward_edge = back_index;
        cap = capacity;
}

};
/*all helper functions for dinic will be defined in this class*/
class Dinic {
public:
    int n;
    //adjacency list
    vector<vector<Edge>> adj;
    /*we track each node’s level during BFS from the source 
    this forms the layered residual graph used in Dinic’s algorithm.*/
    vector<int> level;
    vector<int> start;
    Dinic(int n): n(n), adj(n), level(n), start(n){}

    /*for edge u->v the actual edge is the edge a as it is from u to v
    and with the capacity stated while the edge b is the reverse edge that
    we formed with initially zero capacity*/
    void add_edge(int u, int v, long long cap) {
        Edge a = Edge(v,(int)adj[v].size(),cap);
        Edge b = Edge(u,(int)adj[u].size(),0);
        adj[u].push_back(a);
        adj[v].push_back(b);
    }
    /* s:Source,t:Sink
    traverse from source and mark levels of each node from the source
    and it also ensures positive capacity and finally boolean of 
    level[t]!=-1 is returned which ensures that the path is recahed from
    the source to sink or not
    */
    bool bfs(int s, int t){
        fill(level.begin(), level.end(), -1);
        queue<int> q;
        level[s] = 0;
        q.push(s);
        while(!q.empty()){
            int u = q.front();
            q.pop();
            for (auto &e : adj[u]) {
                if(e.cap > 0 && level[e.next] == -1){
                    level[e.next] = level[u] + 1;
                    q.push(e.next);
                }
            }
        }
        if(level[t] != -1) return true;
        else return false;
    }

    long long dfs(int u, int t, long long flow){
        /*DFS reached the sink so we stop DFS 
        and return the minimuum flow of that path*/
        if(u==t) return flow;

        /*memory address of the start is given and then
        increased every iteration it ensures that DFS
        doesn't keep getting stuck in the same DFS or 
        a node whose all outgoing edges are saturated*/
        for(int &i = start[u];i<adj[u].size();i++){
            Edge &e = adj[u][i];
            //only follows the edges that go one level deeper
            if(e.cap>0 and level[e.next]==level[u]+1){
                long long current_saturation = dfs(e.next,t,min(flow,e.cap));
                if(current_saturation>0){
                    e.cap -= current_saturation;
                    adj[e.next][e.backward_edge].cap += current_saturation;
                    return current_saturation;
                }
            }
        }
        return 0;
    }
    long long max_flow(int s,int t){
        long long flow = 0;
        /*BFS ensures that while there is a path
        from s->t we build the level graph find the
        paths traverse them and multiple DFS calls push
        flow until no more can be sent in this level 
        graph add the saturation flow
        of each path ot get the max flow*/
        while(bfs(s,t)){
            //reset start for every BFS phase
            fill(start.begin(), start.end(), 0);
            while(true){
                long long f = dfs(s,t,LLONG_MAX);
                if(f==0) break;
                flow+=f;
            }
        }
        return flow;
    }

};

int main() {
    int n, m;
    cin >>n>>m;
    Dinic D(n);
    for (int i = 0; i < m; i++) {
        int u, v;
        long long c;
        cin >> u >> v >> c;
        D.add_edge(u, v, c);
    }
    int s,t;
    cin>>s>>t;
    cout<<D.max_flow(s,t)<<endl;
}
