# Training 02 — Engine Architecture & Data-Oriented Design

## Task A — Structure of Arrays (SoA) and Collisions (AABB)

In the classic object-oriented approach (AoS - _Array of Structures_), you
declare a `Box` class with its coordinates `minX, minY, maxX, maxY`, then make
a `std::vector<Box>`. It is readable, but bad for the CPU cache if all you want
is to test collisions on a massive scale, because you may load data of the
structure that you do not need into the cache.

AAA engines use the **SoA (Structure of Arrays)** approach: each attribute goes
into its own array. This lets the processor load only what is needed, and
prepares the code for SIMD (vector computations).

**Goal:** Write a function that finds which bounding boxes (AABB -
_Axis-Aligned Bounding Box_) collide with a target box.

```cpp
#include <vector>
#include <cstdint>

// The data is stored as SoA: all the minX together, all the minY together, etc.
// All the vectors are guaranteed to have exactly the same size.
struct AABB_SoA_Registry {
    std::vector<float> minX;
    std::vector<float> minY;
    std::vector<float> maxX;
    std::vector<float> maxY;
};

struct AABB_Target {
    float minX, minY, maxX, maxY;
};

// Returns a std::vector holding the indices (0, 1, 2...) of all the boxes
// of the registry that intersect the 'target' box.
std::vector<std::uint32_t> findCollisions(
    const AABB_SoA_Registry& registry,
    const AABB_Target& target
);

```

_Math reminder:_ Two boxes A and B intersect on the X axis if
`(A.minX <= B.maxX) && (A.maxX >= B.minX)`. They collide if they intersect on
the X axis **AND** on the Y axis.

**Constraints:**

- Your loop must be as simple and linear as possible, to suit the CPU's
  hardware prefetcher.
- _Engine note:_ Returning a `std::vector` allocates memory, but we accept it
  here to keep the exercise simple (in a real engine, you would pass a
  pre-allocated buffer as an output parameter).

---

## Task B — The "Handles" System (Generational Indices)

This is one of the best-kept secrets of game development. In an engine, we
avoid raw pointers (`Entity*`), because if the `std::vector` of entities is
reallocated, all the pointers become invalid (dangling pointers). We do not use
`std::shared_ptr` either, because maintaining the reference count is too slow
and causes cache misses.

The solution? **Handles**. A Handle is just a 32-bit integer (a `uint32_t`)
split in two:

- The **16 low bits** (index): the index in the array (e.g. slot 5).
- The **16 high bits** (generation): a version number that tells whether the
  object in this slot is still the one we think, or whether the slot was
  recycled for a new object.

**Goal:** Complete the `Handle` structure and the `HandleManager` class.

```cpp
#include <vector>
#include <cstdint>

// A 32-bit Handle holding: Index (16 bits) and Generation (16 bits)
struct Handle {
    std::uint32_t id;
};

// What the manager stores in its array
struct Slot {
    bool active = false;
    std::uint16_t generation = 1;
    // To keep it simple, no real data is stored here (e.g. the Entity):
    // only the lifetime of the slot is managed.
};

class HandleManager {
private:
    std::vector<Slot> slots;
    // (Naive) index to search for a free slot from
    std::uint16_t nextFreeIndex = 0;

public:
    HandleManager(size_t capacity = 1024) {
        slots.resize(capacity);
    }

    // 1. Finds an inactive slot, makes it active, increments its generation,
    // then packs the index and the generation into a 'Handle' and returns it.
    Handle createObject();

    // 2. Checks whether the handle matches the object currently in the slot.
    // You need to extract the index from the handle, check that the slot is active,
    // AND that the handle's generation matches the slot's generation.
    bool isValid(Handle handle) const;

    // 3. Destroys the object: makes the slot inactive (it can be reused).
    void destroyObject(Handle handle);
};

```

_Hint: You will need the bitwise operators (`&`, `|`, `<<`, `>>`) to pack and
unpack your `Handle`._

---

## System Questions (Multithreading & Caches)

**C1 — False Sharing** A developer writes a multithreaded particle system. They
have an atomic array to count the particles processed by each thread:
`std::atomic<int> counters[4];`. Thread 0 increments `counters[0]`, Thread 1
increments `counters[1]`, etc. Although there is technically no _data race_,
the program runs much slower with 4 threads than with a single thread. Briefly
explain why the behavior of the CPU cache causes this slowdown, and how to fix
it simply.

Answer:

Since we use an array of atomic ints, each fetch-and-add is queued: when a
thread wants to fetch-and-add, it is put in a queue, and that is what takes the
most time. What we should do is simply use no atomic and no mutex: if each
thread modifies a value that no other thread touches, it is fine.

**C2 — Double Buffering (Engine Architecture)** In almost every engine (such as
Source 2 or id Tech), the rendering system (Renderer) often runs on a thread
separate from the game logic (Gameplay). However, the logic modifies the
positions of objects while the Renderer tries to read them to draw them. Which
data architecture technique (involving the words "Double" and "Buffer") lets
the Renderer draw the state of the world without being blocked by the Gameplay
thread? Explain the concept in one sentence.

Answer:

The real mechanism: the **pointer swap**.

- **During frame $N$:** the Gameplay thread freely writes the new positions
  into Buffer A, while the Renderer reads Buffer B in peace (it holds the
  complete, frozen state of frame $N-1$). The two work in parallel without ever
  blocking each other.
- **At the end of the frame (synchronization point):** once both threads are
  done, no data is copied: we simply swap the pointers (or flip an index 0 ↔
  1). In frame $N+1$, the Renderer reads Buffer A and the Gameplay thread
  writes into Buffer B.

**The trade-off:** In exchange for lock-free execution and an $O(1)$ swap, we
pay 2× the memory for this shared state, and add 1 frame of latency between the
gameplay computation and its display on screen.
