// https://codeforces.com/problemset/problem/489/b
#include <ios>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace {

void solve()
{
    std::uint16_t n;
    std::cin >> n;

    std::vector<std::uint16_t> a(n);

    for (std::size_t i = 0; i < n; ++i) {
        std::cin >> a[i];
    }
    std::sort(a.begin(), a.end());

    std::uint16_t m;
    std::cin >> m;
    std::vector<std::uint16_t> b(m);

    for (std::size_t i = 0; i < m; ++i) {
        std::cin >> b[i];
    }
    std::sort(b.begin(), b.end());

    std::uint16_t pair = 0;

    std::size_t i = 0;
    std::size_t j = 0;

    while (i < n && j < m) {
        if (std::abs(a[i] - b[j]) <= 1) {
            pair++;
            i++;
            j++;
        } else if (a[i] < b[j]) {
            i++;
        } else {
            j++;
        }
    }

    std::cout << pair << '\n';
}

}

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    solve();
}
