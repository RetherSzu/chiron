// c1_padded.cpp                                                      -*-C++-*-

//@PURPOSE: Fix false sharing: one atomic counter per cache line.
//
//@CLASSES:
//  PaddedCounter: an atomic counter alone on its cache line
//
//@DESCRIPTION: This is `c1.cpp` with each counter padded to a whole cache
// line. Each core now keeps its line to itself, so the threads run in
// parallel: 4 threads take about a quarter of the time of 1.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Measuring the Fix
/// - - - - - - - - - - - - - -
// Build in release mode (the default), then run:
// ```
//  xmake run graphic 02 c1_padded
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

/// Size of a cache line in bytes: 128 on Apple Silicon
/// (`sysctl hw.cachelinesize`), 64 on x86.
constexpr std::size_t k_CACHE_LINE = 128;

/// This simple `struct` puts an atomic counter alone on a cache line:
/// `alignas` aligns it on `k_CACHE_LINE` bytes and pads it to that size.
struct alignas(k_CACHE_LINE) PaddedCounter {
    // DATA

    std::atomic<int> d_value;  // counter, incremented by a single thread
};

static_assert(sizeof(PaddedCounter) == k_CACHE_LINE);

/// One counter per thread, each on its own cache line.
PaddedCounter counters[k_MAX_THREADS];

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
                ++counters[t].d_value;
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
