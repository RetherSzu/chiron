#include <string>
#include <iostream>

class Solution {
  public:
    static bool checkString(std::string s)
    {
        bool flag = false;

        for (const char& ch : s) {
            if (ch == 'a') {
                if (flag) {
                    return false;
                }
            } else {
                flag = true;
            }
        }

        return true;
    }
};


int main()
{
    std::string str = "aabbaaabb";
    if (Solution::checkString(str)) {
        std::cout << "yes\n";
    } else {
        std::cout << "no\n";
    }
}
