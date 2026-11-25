// c2.cpp                                                             -*-C++-*-

//@PURPOSE: Show a data race: two threads increment one `int`, unprotected.
//
//@DESCRIPTION: Two threads each add `k_INCREMENTS` to `counter`, one at a
// time. `counter = counter + 1` is three steps: load, add, store. When both
// threads load the same value, both store that value plus one, and one
// increment is lost: the result is below `2 * k_INCREMENTS`, and changes
// from run to run. This is a data race, which is undefined behavior.
// `c2_mutex.cpp` and `c2_atomic.cpp` are this program with the two standard
// fixes.
//
// `counter` is `volatile` so that the compiler loads and stores it at every
// increment, as written, instead of adding `k_INCREMENTS` to it at once.
// `volatile` does not make the increment atomic: the race is still there.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Losing Increments
/// - - - - - - - - - - - - - -
// Run it a few times, and the result changes:
// ```
//  xmake run graphic 01 c2
// ```
// ThreadSanitizer reports the race:
// ```
//  xmake f -m tsan && xmake run graphic 01 c2
// ```

// std
#include <iostream>
#include <thread>

namespace {

/// Number of increments each thread makes.
constexpr int k_INCREMENTS = 1'000;

/// Counter both threads increment, with nothing to stop them racing.
volatile int counter = 0;

/// Add 1 to `counter` the specified `n` times.
void increment(int n)
{
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
