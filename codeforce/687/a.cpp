// https://codeforces.com/problemset/problem/1676/g
#include <ios>
#include <iostream>
#include <vector>
#include <queue>

namespace {

void solve()
{
    std::size_t n;
    std::size_t m;

    std::cin >> n >> m;
    std::vector<std::vector<std::size_t>> adj(n + 1);

    for (std::size_t i = 0; i < m; ++i) {
        std::size_t u;
        std::size_t v;
        std::cin >> u >> v;
        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    std::queue<std::size_t> queue;
    std::vector<int>        color(n + 1);  // 0: unvisited, 1: in A, 2: in B

    for (std::size_t start = 1; start <= n; ++start) {
        if (color[start] != 0) {
            continue;
        }

        color[start] = 1;
        queue.push(start);

        while (!queue.empty()) {
            std::size_t node = queue.front();
            queue.pop();
            for (std::size_t next : adj[node]) {
                if (color[next] == 0) {
                    color[next] = 3 - color[node];
                    queue.push(next);
                } else if (color[next] == color[node]) {
                    std::cout << -1 << '\n';
                    return;
                }
            }
        }
    }

    std::vector<std::size_t> A;
    std::vector<std::size_t> B;

    for (std::size_t i = 1; i <= n; ++i) {
        if (color[i] == 1) {
            A.push_back(i);
        } else if (color[i] == 2) {
            B.push_back(i);
        }
    }

    std::cout << A.size() << '\n';
    for (std::size_t i = 0; i < A.size(); ++i) {
        std::cout << A[i] << ' ';
    }
    std::cout << '\n';

    std::cout << B.size() << '\n';
    for (std::size_t i = 0; i < B.size(); ++i) {
        std::cout << B[i] << ' ';
    }
    std::cout << '\n';
}

}

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    solve();

    return 0;
}
