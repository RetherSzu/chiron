// https://codeforces.com/problemset/problem/474/B
#include <ios>
#include <iostream>
#include <vector>

namespace {

void solve()
{
    std::uint32_t n;
    std::cin >> n;

    std::vector<std::uint32_t> pref(n);
    std::cin >> pref[0];
    for (std::size_t i = 1; i < n; ++i) {
        std::cin >> pref[i];
        pref[i] += pref[i - 1];
    }

    std::uint32_t m;
    std::cin >> m;

    for (std::size_t i = 0; i < m; ++i) {
        std::uint32_t q;
        std::cin >> q;

        std::size_t low  = 0;
        std::size_t high = pref.size();

        while (low < high) {
            std::size_t mid = low + ((high - low) / 2);

            if (pref[mid] >= q) {
                high = mid;
            } else {
                low = mid + 1;
            }
        }

        std::cout << low + 1 << '\n';
    }
}

}

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    solve();
}
