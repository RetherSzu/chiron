#include <vector>
#include <iostream>


class ProductOfNumbers {
  private:
    std::vector<int> numbers;

  public:
    ProductOfNumbers() {}

    void add(int num)
    {
        if (num == 0) {
            numbers.clear();
        }
        numbers.push_back(num);
    }

    int getProduct(int k)
    {
        std::size_t n = numbers.size();
        if (n - k < 0) {
            return 0;
        }

        int ans = 1;
        for (int i = k; i > 0; --i) {
            ans *= numbers[n - i];
        }
        return ans;
    }
};


int main()
{
    ProductOfNumbers productOfNumbers = ProductOfNumbers();
    productOfNumbers.add(3);  // [3]
    productOfNumbers.add(0);  // [3,0]
    productOfNumbers.add(2);  // [3,0,2]
    productOfNumbers.add(5);  // [3,0,2,5]
    productOfNumbers.add(4);  // [3,0,2,5,4]
    std::cout << productOfNumbers.getProduct(2)
              << std::endl;  // return 20. The product of the last 2 numbers is
                             // 5 * 4 = 20
    std::cout << productOfNumbers.getProduct(3)
              << std::endl;  // return 40. The product of the last 3 numbers is
                             // 2 * 5 * 4 = 40
    std::cout << productOfNumbers.getProduct(4)
              << std::endl;   // return 0. The product of the last 4
                              // numbers is 0 * 2 * 5 * 4 = 0
    productOfNumbers.add(8);  // [3,0,2,5,4,8]
    std::cout << productOfNumbers.getProduct(2)
              << std::endl;  // return 32. The product of the last 2 numbers is
                             // 4 * 8 = 32
}
