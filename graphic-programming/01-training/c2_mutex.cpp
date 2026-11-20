// c2_mutex.cpp                                                       -*-C++-*-

//@PURPOSE: Fix the data race of `c2.cpp` with a mutex.
//
//@DESCRIPTION: This is `c2.cpp` with `increment` holding `mutex` for its
// whole loop. The two threads never run the loop at the same time: the
// second one waits until the first is done, so no increment is lost and the
// result is always `2 * k_INCREMENTS`. The threads also no longer run in
// parallel. Locking once per increment would let them interleave, but would
// pay for a lock and an unlock at every increment.
//
// The mutex alone makes the program correct. `counter` is still `volatile`
// only so that the loop is the same as in `c2.cpp`.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Losing No Increment
/// - - - - - - - - - - - - - - -
// ```
//  xmake run 01 c2_mutex
// ```

// std
#include <iostream>
#include <mutex>
#include <thread>

namespace {

/// Number of increments each thread makes.
constexpr int k_INCREMENTS = 1'000;

/// Mutex that a thread holds while it increments `counter`.
std::mutex mutex;

/// Counter both threads increment, under `mutex`.
volatile int counter = 0;

/// Add 1 to `counter` the specified `n` times, holding `mutex` throughout.
void increment(int n)
{
    const std::lock_guard<std::mutex> lock(mutex);
    for (int i = 0; i < n; ++i) {
        counter = counter + 1;
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
