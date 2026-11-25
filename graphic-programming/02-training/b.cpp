// b.cpp                                                              -*-C++-*-

//@PURPOSE: Provide generational handles: indices that know they are stale.
//
//@CLASSES:
//  Handle: a 16-bit slot index and a 16-bit generation, in 32 bits
//  Slot: the state of one slot of a `HandleManager`
//  HandleManager: hands out handles to slots, and tells which are valid
//
//@DESCRIPTION: An engine avoids raw pointers to its objects (`Entity*`):
// when the `std::vector` holding the objects is reallocated, every pointer
// dangles. It avoids `std::shared_ptr` too: maintaining the reference count
// is too slow and causes cache misses. It hands out handles instead. A
// `Handle` is a 32-bit integer split in two:
//
//: o The 16 low bits are the index of a slot in the manager's array.
//:
//: o The 16 high bits are the generation of that slot when the handle was
//:   made.
//
// A slot's generation goes up every time the slot is given to a new object.
// A handle to an object that was destroyed, and whose slot was given to
// another object, no longer has its slot's generation: `is_valid` returns
// `false` for it, where a pointer would silently point at the new object.
// After 65535 reuses of one slot, the generation comes back to the same
// value, and an old handle to that slot looks valid again.
//
// A generation is never 0 (it goes from 65535 back to 1), so a `Handle`
// whose id is 0 is never valid: `create_object` returns it when no slot is
// free. The free slots are kept on a stack of indices, so `create_object`
// and `destroy_object` take constant time, and the slot freed last is the
// first one reused.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Detecting a Stale Handle
///- - - - - - - - - - - - - - - - - -
// ```
//  HandleManager manager;
//
//  const Handle a = manager.create_object();
//  assert(manager.is_valid(a));
//
//  manager.destroy_object(a);
//  assert(!manager.is_valid(a));
//
//  const Handle b = manager.create_object();  // reuses the slot of 'a'
//  assert(manager.is_valid(b));
//  assert(!manager.is_valid(a));  // 'a' has the slot's previous generation
// ```
// `main` runs this example:
// ```
//  xmake run graphic 02 b
// ```

// std
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

/// This simple `struct` is a handle to a slot of a `HandleManager`: the
/// slot's index in its 16 low bits, and the slot's generation when the
/// handle was made in its 16 high bits.
struct Handle {
    // DATA

    std::uint32_t d_id;  // `generation << 16 | index`; 0 is never valid
};

/// This simple `struct` holds the state of one slot of a `HandleManager`. To
/// keep it simple, it stores no real data (such as the `Entity`): only the
/// lifetime of the slot is managed.
struct Slot {
    // DATA

    bool          d_active     = false;  // `true` while an object holds it
    std::uint16_t d_generation = 1;      // goes up at each reuse, never 0
};

                         // ===================
                         // class HandleManager
                         // ===================

/// This mechanism class hands out handles to a fixed number of slots, and
/// tells whether a handle still refers to the object it was made for.
class HandleManager final {
  private:
    // DATA

    std::vector<Slot>          d_slots;         // every slot, free or not
    std::vector<std::uint16_t> d_free_indices;  // free slots, next at back

  public:
    // CREATORS

    /// Create a `HandleManager` object with the specified `capacity` slots,
    /// 1024 by default, all free. The behavior is undefined unless
    /// `capacity <= 65536`.
    explicit HandleManager(std::size_t capacity = 1024);

    // MANIPULATORS

    /// Take a free slot, make it active, increment its generation, and
    /// return a handle holding the slot's index and new generation. Return a
    /// handle whose id is 0, which is never valid, if no slot is free.
    [[nodiscard]] Handle create_object();

    /// Make the slot of the specified `handle` inactive, so that it can be
    /// reused, if `handle` is valid; do nothing otherwise.
    void destroy_object(Handle handle);

    // ACCESSORS

    /// Return `true` if the slot of the specified `handle` is active and has
    /// the generation of `handle`, and `false` otherwise.
    [[nodiscard]] bool is_valid(Handle handle) const;
};

                         // -------------------
                         // class HandleManager
                         // -------------------

// CREATORS

HandleManager::HandleManager(std::size_t capacity)
: d_slots(capacity)
{
    assert(capacity <= 65536);

    // From the last index down to 0, so that slot 0 is at the back of the
    // stack and is given out first.
    d_free_indices.reserve(capacity);
    for (std::size_t i = capacity; i > 0; --i) {
        d_free_indices.push_back(static_cast<std::uint16_t>(i - 1));
    }
}

// MANIPULATORS

Handle HandleManager::create_object()
{
    if (d_free_indices.empty()) {
        return Handle{0};                                             // RETURN
    }

    const std::uint16_t index = d_free_indices.back();
    d_free_indices.pop_back();
    Slot& slot = d_slots[index];

    slot.d_active = true;
    ++slot.d_generation;
    if (slot.d_generation == 0) {
        slot.d_generation = 1;
    }

    const Handle handle{(static_cast<std::uint32_t>(slot.d_generation) << 16) |
                        index};
    return handle;
}

void HandleManager::destroy_object(Handle handle)
{
    if (!is_valid(handle)) {
        return;                                                       // RETURN
    }

    const auto index        = static_cast<std::uint16_t>(handle.d_id & 0xFFFF);
    d_slots[index].d_active = false;
    d_free_indices.push_back(index);
}

// ACCESSORS

bool HandleManager::is_valid(Handle handle) const
{
    const auto generation = static_cast<std::uint16_t>(handle.d_id >> 16);
    const auto index      = static_cast<std::uint16_t>(handle.d_id & 0xFFFF);

    if (index >= d_slots.size()) {
        return false;                                                 // RETURN
    }

    const Slot& slot = d_slots[index];
    return slot.d_active && slot.d_generation == generation;
}

}  // close unnamed namespace

int main()
{
    HandleManager manager;

    const Handle a = manager.create_object();
    std::cout << std::boolalpha << "a = create_object() : a valid "
              << manager.is_valid(a) << '\n';

    manager.destroy_object(a);
    std::cout << "destroy_object(a)   : a valid " << manager.is_valid(a)
              << '\n';

    // The low 16 bits of a handle are its slot's index.
    const Handle b = manager.create_object();
    std::cout << "b = create_object() : b valid " << manager.is_valid(b)
              << ", a valid " << manager.is_valid(a) << ", same slot "
              << ((a.d_id & 0xFFFF) == (b.d_id & 0xFFFF)) << '\n';
}
