// https://codeforces.com/problemset/problem/1676/g
#include <ios>
#include <iostream>
#include <cstdint>
#include <vector>
#include <string>

namespace {

void solve()
{
    std::size_t n;
    std::cin >> n;
    std::vector<std::size_t> parent(n + 1);

    for (std::size_t i = 2; i <= n; ++i) {
        std::cin >> parent[i];
    }

    std::string s;
    std::cin >> s;
    std::vector<int> bal(n + 1);
    for (std::size_t i = 1; i <= n; ++i) {
        bal[i] = (s[i - 1] == 'W') ? 1 : -1;
    }

    std::size_t ans = 0;
    for (std::size_t i = n; i >= 2; --i) {
        if (bal[i] == 0) {
            ans++;
        }
        bal[parent[i]] += bal[i];
    }

    if (bal[1] == 0) {
        ans++;
    }

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
