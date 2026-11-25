# Training 01 - Engine & Systems Programming

## Task A — Entity Compaction (Cache-Friendly Data Compaction)

In a data-oriented architecture (such as an Entity Component System, or ECS),
CPU performance depends heavily on data being contiguous in memory, to avoid
cache misses.

You are given an array of `Entity` structures. During a game frame, some
entities were marked as "inactive" (destroyed). Write a function that removes
the inactive entities and packs the active entities at the start of the array,
**keeping their original order**.

```cpp
#include <vector>
#include <cstdint>
#include <cstddef>

struct Entity {
    std::uint32_t id;
    bool isActive;
    // ... other data (position, velocity, etc.)
};

// Returns the new number of active entities.
std::size_t compactEntities(std::vector<Entity>& entities) {
    std::size_t wi = 0;
    for (std::size_t i = 0; i < entities.size(); ++i) {
        if (entities[i].isActive) {
            if (wi != i) {
                entities[wi] = std::move(entities[i]);
            }
            wi++;
        }
    }
    entities.erase(entities.begin() + wi, entities.end());
    return wi;
}

```

**Constraints and expectations:**

- You must modify the vector in place.
- Target complexity: $O(n)$ time and $O(1)$ extra space.
- Do not create a new vector or a temporary array.

## Task B — Basic Linear Allocator (Linear Memory Allocator)

Classic dynamic allocations (`new`, `malloc`) are too slow and fragment memory,
which is a major flaw in the main loop of a game engine. As a result, engine
programmers often write their own custom memory allocators.

Implement a very basic `LinearAllocator` class. A linear allocator allocates
memory by simply moving a pointer (or an offset) forward in a large block of
memory allocated up front, at startup. It cannot free memory individually: it
can only reset everything at once (which is very useful for temporary
allocations that last for a single frame).

```cpp
#include <cstddef>

class LinearAllocator {
public:
    // Initializes the allocator with a total size in bytes
    LinearAllocator(std::size_t totalSize);

    // Frees the large main block of memory
    ~LinearAllocator();

    // Allocates 'size' bytes. Returns nullptr if there is no room left.
    // For this simple exercise, ignore alignment constraints.
    void* allocate(std::size_t size);

    // Resets the allocator (virtually erases everything by setting the offset back to zero)
    void clear();

private:
    // Your member variables here
};

```

**Usage example:**

```cpp
LinearAllocator alloc(1024); // Allocates a block of 1024 bytes on the heap
void* p1 = alloc.allocate(100); // Success
void* p2 = alloc.allocate(1000); // Must return nullptr (100 + 1000 > 1024)
alloc.clear();
void* p3 = alloc.allocate(500); // Success, starts again from the beginning of the block

```

## System Questions (Hardware & Concurrency)

Answer in a few short, precise sentences to test your software theory.

**C1 — Iteration and CPU Cache** Why is iterating over a
`std::vector<GameObject*>` (an array of pointers to objects) generally much
slower than iterating over a `std::vector<GameObject>` (an array holding the
objects themselves, allocated by value), even if the math executed inside the
loop is exactly the same?

Answer:

In a `std::vector<GameObject>`, all the objects are stored physically side by
side in RAM. When the CPU wants to read the first object, it does not fetch
just that object: it brings a whole "block" of data (a cache line, often 64
bytes) into its L1/L2 cache. By the time the loop moves on to the 2nd, 3rd and
4th object, the data is already in the processor. Access is almost instant.

But in a `std::vector<GameObject*>`, the array holds the pointers side by side,
which is fast to read. The objects themselves (created with `new`), however,
are scattered randomly in memory (the heap). At each iteration of the loop, the
processor reads the address, looks for the object in its cache, does not find
it (cache miss), and must stall for hundreds of clock cycles to make a round
trip to the very slow RAM.

**C2 — Basic Concurrency** Two threads (Thread A and Thread B) access a global
variable `int counter = 0;` at the same time. Thread A runs the loop
`counter++;` 1000 times. Thread B runs the same loop `counter++;` 1000 times.
At the end of the program, the value of `counter` is 1845 instead of 2000.
Briefly explain this phenomenon (data race) and name a standard C++ way (for
example, one introduced in C++11) to guarantee that the final result is always
exactly 2000, without using a mutex.

The nuance is that using an `atomic` variable is precisely the hardware
solution to this problem. A true atomic operation (such as a _fetch-and-add_)
forces the processor to lock the data at the hardware level: the read, the
addition and the write become a single indivisible operation. The two threads
can never both read 1337; the processor puts one of them on hold for a fraction
of a nanosecond, which guarantees that it does read 1338 and moves on to 1339.
