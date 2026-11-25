// https://codeforces.com/problemset/problem/1676/g
#include <ios>
#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <utility>

namespace {

struct Node {
    char                     d_color;
    std::vector<std::size_t> d_childs;
};

std::pair<std::size_t, std::size_t>
traversal(std::vector<Node>& nodes, std::size_t childIndex, std::size_t& ans)
{
    std::pair<std::size_t, std::size_t> bw = {0, 0};

    if (nodes[childIndex].d_color == 'W') {
        bw.first += 1;
    } else {
        bw.second += 1;
    }

    for (std::size_t i = 0; i < nodes[childIndex].d_childs.size(); ++i) {
        std::pair<std::size_t, std::size_t> bwc = traversal(
            nodes, nodes[childIndex].d_childs[i], ans);
        bw.first += bwc.first;
        bw.second += bwc.second;
    }

    if (bw.first == bw.second) {
        ans++;
    }

    return bw;
}

void solve()
{
    std::size_t n;
    std::cin >> n;
    std::vector<Node> nodes(n);

    for (std::size_t i = 0; i < n; ++i) {
        nodes[i] = Node{.d_color = 'W', .d_childs = {}};
    }

    for (std::size_t i = 0; i < n - 1; ++i) {
        std::size_t iNode;
        std::cin >> iNode;
        nodes[iNode - 1].d_childs.push_back(i + 1);
    }

    std::string colors;
    std::cin >> colors;

    for (std::size_t i = 0; i < n; ++i) {
        nodes[i].d_color = colors[i];
    }

    std::size_t ans = 0;

    traversal(nodes, 0, ans);
    std::cout << ans << '\n';
}

}

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::uint32_t t;
    std::cin >> t;

    while (t > 0) {
        solve();
        t--;
    }

    return 0;
}
