// c1.cpp                                                             -*-C++-*-

//@PURPOSE: Show false sharing: atomic counters packed on one cache line.
//
//@DESCRIPTION: Each thread processes its share of `k_PARTICLES` and counts
// them in its own counter, `counters[t]`. The threads never touch the same
// counter, but the counters sit next to each other on the same cache line,
// so every increment by one core invalidates that line in the other cores'
// caches, and the line keeps bouncing between them: 4 threads end up slower
// than 1. `c1_padded.cpp` is the same program with one counter per line.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Measuring False Sharing
/// - - - - - - - - - - - - - - - - -
// Build in release mode (the default), then run:
// ```
//  xmake run 02 c1
// ```

// std
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

namespace {

/// Maximum number of threads, and so of counters.
constexpr int k_MAX_THREADS = 4;

/// Particles processed in total, split evenly between the threads.
constexpr int k_PARTICLES = 10'000'000;

/// Number of runs each printed time is the mean of.
constexpr int k_RUNS = 100;

/// One counter per thread, packed next to each other in memory.
std::atomic<int> counters[k_MAX_THREADS];

/// Start the specified `num_threads` threads, thread `t` processing
/// `k_PARTICLES / num_threads` particles and counting them in `counters[t]`,
/// and wait for all of them to finish. Return the elapsed time in
/// milliseconds. The behavior is undefined unless
/// `1 <= num_threads <= k_MAX_THREADS`.
double process_particles(int num_threads)
{
    assert(1 <= num_threads && num_threads <= k_MAX_THREADS);

    const int  per_thread = k_PARTICLES / num_threads;
    const auto start      = std::chrono::steady_clock::now();

    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(num_threads));
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, per_thread] {
            for (int i = 0; i < per_thread; ++i) {
                // ... update particle i ...
                ++counters[t];
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }

    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

/// Return the mean time, in milliseconds, of `k_RUNS` calls to
/// `process_particles` with the specified `num_threads`.
double average_time(int num_threads)
{
    double total = 0.0;
    for (int run = 0; run < k_RUNS; ++run) {
        total += process_particles(num_threads);
    }
    return total / k_RUNS;
}

}  // close unnamed namespace

int main()
{
    const double one_thread   = average_time(1);
    const double four_threads = average_time(4);

    std::cout << "1 thread  : " << one_thread << " ms\n"
              << "4 threads : " << four_threads << " ms\n";
}
