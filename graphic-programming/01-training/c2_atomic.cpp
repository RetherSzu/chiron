// c2_atomic.cpp                                                      -*-C++-*-

//@PURPOSE: Fix the data race of `c2.cpp` with an atomic counter.
//
//@DESCRIPTION: This is `c2.cpp` with `counter` a `std::atomic<int>`. Its
// `++` is one indivisible read-modify-write, a fetch-and-add: the CPU does
// not let another core touch the counter between the load and the store, so
// two threads can never both read the same value. No increment is lost and
// the result is always `2 * k_INCREMENTS`, without a mutex. The threads
// still run in parallel, but each increment makes the cores pass the
// counter's cache line to each other.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Losing No Increment
/// - - - - - - - - - - - - - - -
// ```
//  xmake run graphic 01 c2_atomic
// ```

// std
#include <atomic>
#include <iostream>
#include <thread>

namespace {

/// Number of increments each thread makes.
constexpr int k_INCREMENTS = 1'000;

/// Counter both threads increment, atomically.
std::atomic<int> counter(0);

/// Add 1 to `counter` the specified `n` times.
void increment(int n)
{
    for (int i = 0; i < n; ++i) {
        ++counter;
    }
}

}  // close unnamed namespace

int main()
{
    std::thread t1(increment, k_INCREMENTS);
    std::thread t2(increment, k_INCREMENTS);

    t1.join();
    t2.join();

    std::cout << "expected : " << 2 * k_INCREMENTS << '\n'
              << "actual   : " << counter << '\n';
}
