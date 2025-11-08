#include <bits/stdc++.h>
using namespace std;

struct Edge {
    int next;
    double cap;
};

int main() {
    ifstream fin("residual.txt");
    if (!fin.is_open()) {
        cerr << "Error: could not open residual.txt\n";
        return 1;
    }

    int n;
    fin >> n;
    vector<vector<Edge>> adj(n);

    int u, v;
    double cap;
    while (fin >> u >> v >> cap) {
        adj[u].push_back({v, cap});
    }
    fin.close();
    int s = 0; 
    vector<bool> visited(n, false);
    stack<int> st;
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
    cout << "Edges in the Min Cut:\n";
    for (int u = 0; u < n; ++u) {
        if (visited[u]) {
            for (auto &e : adj[u]) {
                if (!visited[e.next] && e.cap <= 1e-9) {
                    cout << u << " -> " << e.next << endl;
                }
            }
        }
    }
    ofstream mask("mask.txt");
    for (int i = 0; i < n; i++)
        mask << visited[i] << " ";
    mask.close();

    cout << "\nMask written to mask.txt\n";
    return 0;
}
