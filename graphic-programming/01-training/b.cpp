// b.cpp                                                              -*-C++-*-

//@PURPOSE: Provide a linear allocator: allocate by moving an offset forward.
//
//@CLASSES:
//  LinearAllocator: hands out the bytes of one block, frees them all at once
//
//@DESCRIPTION: `new` and `malloc` are too slow to call in the main loop of a
// game engine, and they fragment memory, so engine programmers often write
// allocators of their own. A `LinearAllocator` allocates one large block
// when it is created, then serves each allocation by moving an offset
// forward in that block. It cannot free one allocation: `clear` frees them
// all at once, by putting the offset back to zero. This suits temporary
// allocations that live for a single frame.
//
// For this simple exercise, allocations are not aligned: `allocate` returns
// the next free byte, whatever its address.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Allocating, Then Clearing
/// - - - - - - - - - - - - - - - - - -
// ```
//  LinearAllocator allocator(1024);  // one block of 1024 bytes on the heap
//
//  void* p1 = allocator.allocate(100);   // succeeds
//  void* p2 = allocator.allocate(1000);  // null: 100 + 1000 > 1024
//
//  allocator.clear();
//  void* p3 = allocator.allocate(500);   // succeeds, from the block's start
//  assert(p3 == p1);
// ```
// `main` runs this example:
// ```
//  xmake run 01 b
// ```

// std
#include <cstddef>
#include <cstdlib>
#include <iostream>

namespace {

                         // =====================
                         // class LinearAllocator
                         // =====================

/// This mechanism class hands out the bytes of one block, allocated when it
/// is created, by moving an offset forward, and frees them all at once.
class LinearAllocator final {
  private:
    // DATA

    std::byte*  d_pool_p;    // the block, owned; null if it was not had
    std::size_t d_offset;    // bytes allocated so far, at most `d_capacity`
    std::size_t d_capacity;  // size of the block in bytes, 0 if it is null

    // NOT IMPLEMENTED

    LinearAllocator(const LinearAllocator&)            = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;

  public:
    // CREATORS

    /// Create a `LinearAllocator` object serving the bytes of one block of
    /// the specified `total_size` bytes, allocated on the heap. If the block
    /// cannot be allocated, every allocation of one byte or more fails.
    explicit LinearAllocator(std::size_t total_size) noexcept;

    /// Destroy this object, and free its block.
    ~LinearAllocator();

    // MANIPULATORS

    /// Return the address of the specified `size` next free bytes of the
    /// block, or null if fewer than `size` bytes are free. The bytes stay
    /// valid until `clear` is called or this object is destroyed. The
    /// address is not aligned.
    [[nodiscard]] void* allocate(std::size_t size) noexcept;

    /// Make every byte of the block free again. Every address returned by
    /// `allocate` so far is then invalid.
    void clear() noexcept;
};

                         // ---------------------
                         // class LinearAllocator
                         // ---------------------

// CREATORS

LinearAllocator::LinearAllocator(std::size_t total_size) noexcept
: d_pool_p(static_cast<std::byte*>(std::malloc(total_size)))
, d_offset(0)
, d_capacity(d_pool_p == nullptr ? 0 : total_size)
{
}

LinearAllocator::~LinearAllocator()
{
    std::free(d_pool_p);
}

// MANIPULATORS

void* LinearAllocator::allocate(std::size_t size) noexcept
{
    if (size > d_capacity - d_offset) {
        return nullptr;                                               // RETURN
    }
    const std::size_t old_offset = d_offset;
    d_offset += size;
    return d_pool_p + old_offset;
}

void LinearAllocator::clear() noexcept
{
    d_offset = 0;
}

}  // close unnamed namespace

int main()
{
    LinearAllocator allocator(1024);  // one block of 1024 bytes on the heap

    const void* const p1 = allocator.allocate(100);  // succeeds
    const void* const p2 = allocator.allocate(
        1000);  // null: 100 + 1000 > 1024
    allocator.clear();
    const void* const p3 = allocator.allocate(
        500);  // succeeds, from the start

    std::cout << std::boolalpha << "p1 = allocate(100)  : allocated "
              << (p1 != nullptr) << '\n'
              << "p2 = allocate(1000) : allocated " << (p2 != nullptr) << '\n'
              << "clear()\n"
              << "p3 = allocate(500)  : allocated " << (p3 != nullptr)
              << ", same address as p1 " << (p3 == p1) << '\n';
}
