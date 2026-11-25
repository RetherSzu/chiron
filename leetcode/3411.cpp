#include <vector>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <numeric>

namespace {


class Solution {
  public:
    static int max_length(const std::vector<int>& nums)
    {
        std::size_t n = nums.size();
        if (n < 2) {
            return (n == 1 && nums[0] == 1) ? 1 : 0;
        }

        int         maxLen     = 2;
        std::size_t left       = 0;
        int         windowProd = 1;

        for (std::size_t right = 0; right < n; ++right) {
            int val = nums[right];
            while (std::gcd(windowProd, val) > 1) {
                windowProd /= nums[left];
                left++;
            }

            windowProd *= val;
            maxLen = std::max(maxLen, static_cast<int>(right - left + 1));
        }


        return maxLen;
    }
};

}

int main()
{
    auto x = Solution::max_length({1, 2, 1, 2, 1, 1, 1});
    std::cout << x;
    return 0;
}
