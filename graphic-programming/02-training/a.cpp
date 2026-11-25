// a.cpp                                                              -*-C++-*-

//@PURPOSE: Find the boxes that collide with a target, stored as SoA.
//
//@CLASSES:
//  AabbSoaRegistry: axis-aligned bounding boxes, one array per coordinate
//  AabbTarget: the axis-aligned bounding box the registry is tested against
//
//@DESCRIPTION: The classic object-oriented layout is an Array of Structures
// (AoS): a `Box` holding its `minX, minY, maxX, maxY`, in a
// `std::vector<Box>`. It is readable, but a test that needs only some of the
// fields still loads whole boxes into the cache. A Structure of Arrays (SoA)
// puts each coordinate in an array of its own instead: `AabbSoaRegistry`
// holds every `min_x` together, every `min_y` together, and so on. A loop
// over the boxes then reads each array from start to end, which the
// hardware prefetcher follows easily, and can test several boxes at once
// with SIMD.
//
// `find_collisions` returns the index of every box of a registry that
// collides with a target box. Two boxes `a` and `b` intersect on the x axis
// if `a.min_x <= b.max_x && a.max_x >= b.min_x`, and collide if they
// intersect on both the x and the y axes.
//
///SIMD
///----
// When the code is compiled for AVX (`__AVX__`, as with `-mavx` on x86), the
// loop tests 8 boxes per iteration. It loads 8 values of each coordinate
// into a `__m256`, compares them with the target's coordinates (each copied
// into all 8 lanes), ANDs the 4 comparisons, and turns the result into a
// mask of 8 bits, one per box (`_mm256_movemask_ps`). Each set bit is a box
// that collides: `std::countr_zero` gives the position of the lowest one,
// and `mask &= mask - 1` clears it.
//
// A scalar loop then tests the boxes left over, fewer than 8. Without AVX,
// as on Apple Silicon, the scalar loop tests every box.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Finding the Boxes a Target Hits
/// - - - - - - - - - - - - - - - - - - - - -
// Ten unit boxes in a row, box `i` spanning `[i, i + 1]` on x and `[0, 1]` on
// y, and a target spanning `[2.5, 8.5]` on x and `[0.25, 0.75]` on y:
// ```
//  AabbSoaRegistry registry;
//  for (int i = 0; i < 10; ++i) {
//      const float x = static_cast<float>(i);
//      registry.d_min_x.push_back(x);
//      registry.d_min_y.push_back(0.0f);
//      registry.d_max_x.push_back(x + 1.0f);
//      registry.d_max_y.push_back(1.0f);
//  }
//
//  const AabbTarget                 target = {2.5f, 0.25f, 8.5f, 0.75f};
//  const std::vector<std::uint32_t> hits   = find_collisions(registry,
//  target);
//  // 'hits' is 2, 3, 4, 5, 6, 7, 8
// ```
// `main` runs this example:
// ```
//  xmake run graphic 02 a
// ```

// std
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#ifdef __AVX__
#include <bit>
#include <immintrin.h>
#endif

namespace {

/// This simple `struct` holds axis-aligned bounding boxes as a Structure of
/// Arrays: box `i` is `[d_min_x[i], d_max_x[i]]` on x and
/// `[d_min_y[i], d_max_y[i]]` on y. The four arrays have the same size.
struct AabbSoaRegistry {
    // DATA

    std::vector<float> d_min_x;  // left edge of each box
    std::vector<float> d_min_y;  // bottom edge of each box
    std::vector<float> d_max_x;  // right edge of each box
    std::vector<float> d_max_y;  // top edge of each box
};

/// This simple `struct` holds the axis-aligned bounding box that the boxes
/// of an `AabbSoaRegistry` are tested against.
struct AabbTarget {
    // DATA

    float d_min_x;  // left edge
    float d_min_y;  // bottom edge
    float d_max_x;  // right edge
    float d_max_y;  // top edge
};

/// Return the index of every box of the specified `registry` that collides
/// with the specified `target`, in increasing order. The behavior is
/// undefined unless the four arrays of `registry` have the same size.
[[nodiscard]] std::vector<std::uint32_t>
find_collisions(const AabbSoaRegistry& registry, const AabbTarget& target)
{
    const std::size_t          n = registry.d_max_x.size();
    std::vector<std::uint32_t> in_box;
    in_box.reserve(n);

    const float* const min_x = registry.d_min_x.data();
    const float* const min_y = registry.d_min_y.data();
    const float* const max_x = registry.d_max_x.data();
    const float* const max_y = registry.d_max_y.data();

    std::size_t i = 0;

#ifdef __AVX__
    // The target's coordinates, each copied into all 8 lanes.
    const __m256 target_min_x = _mm256_set1_ps(target.d_min_x);
    const __m256 target_min_y = _mm256_set1_ps(target.d_min_y);
    const __m256 target_max_x = _mm256_set1_ps(target.d_max_x);
    const __m256 target_max_y = _mm256_set1_ps(target.d_max_y);

    for (; i + 8 <= n; i += 8) {
        const __m256 box_min_x = _mm256_loadu_ps(min_x + i);
        const __m256 box_min_y = _mm256_loadu_ps(min_y + i);
        const __m256 box_max_x = _mm256_loadu_ps(max_x + i);
        const __m256 box_max_y = _mm256_loadu_ps(max_y + i);

        const __m256 cmp_min_x = _mm256_cmp_ps(box_min_x,
                                               target_max_x,
                                               _CMP_LE_OQ);
        const __m256 cmp_min_y = _mm256_cmp_ps(box_min_y,
                                               target_max_y,
                                               _CMP_LE_OQ);
        const __m256 cmp_max_x = _mm256_cmp_ps(box_max_x,
                                               target_min_x,
                                               _CMP_GE_OQ);
        const __m256 cmp_max_y = _mm256_cmp_ps(box_max_y,
                                               target_min_y,
                                               _CMP_GE_OQ);

        const __m256 valid_x = _mm256_and_ps(cmp_min_x, cmp_max_x);
        const __m256 valid_y = _mm256_and_ps(cmp_min_y, cmp_max_y);
        const __m256 valid   = _mm256_and_ps(valid_x, valid_y);

        // One bit per box, set if the box collides.
        auto mask = static_cast<unsigned>(_mm256_movemask_ps(valid));
        while (mask != 0) {
            const auto bit = static_cast<unsigned>(std::countr_zero(mask));
            in_box.push_back(static_cast<std::uint32_t>(i + bit));
            mask &= mask - 1;  // clear the lowest set bit
        }
    }
#endif

    for (; i < n; ++i) {
        if ((min_x[i] <= target.d_max_x) && (max_x[i] >= target.d_min_x) &&
            (min_y[i] <= target.d_max_y) && (max_y[i] >= target.d_min_y)) {
            in_box.push_back(static_cast<std::uint32_t>(i));
        }
    }
    return in_box;
}

}  // close unnamed namespace

int main()
{
    AabbSoaRegistry registry;
    for (int i = 0; i < 10; ++i) {
        const float x = static_cast<float>(i);
        registry.d_min_x.push_back(x);
        registry.d_min_y.push_back(0.0f);
        registry.d_max_x.push_back(x + 1.0f);
        registry.d_max_y.push_back(1.0f);
    }

    const AabbTarget                 target = {2.5f, 0.25f, 8.5f, 0.75f};
    const std::vector<std::uint32_t> hits = find_collisions(registry, target);

#ifdef __AVX__
    std::cout << "with AVX    :";
#else
    std::cout << "without AVX :";
#endif
    for (const std::uint32_t index : hits) {
        std::cout << ' ' << index;
    }
    std::cout << '\n';
}
