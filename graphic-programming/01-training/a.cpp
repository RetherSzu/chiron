// a.cpp                                                              -*-C++-*-

//@PURPOSE: Remove the inactive entities of a vector, in place and in order.
//
//@CLASSES:
//  Entity: one entity of a game, active or destroyed
//
//@DESCRIPTION: This component provides `compact_entities`, which removes the
// inactive entities of a `std::vector<Entity>` and moves the active ones to
// its front, in their original order, without allocating: O(n) time and O(1)
// extra space. An Entity Component System does this after a frame destroyed
// some entities, so that the next loops over them read contiguous memory
// (see `c1.cpp`).
//
///Algorithm
///---------
// One pass with two indices: `i` visits every entity, and `wi`, the write
// index, is where the next active entity goes. `wi` never passes `i`, so an
// active entity is only ever copied towards the front, onto an inactive or
// already copied entity. At the end, the vector is cut after the last active
// entity.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Compacting After a Frame
///- - - - - - - - - - - - - - - - - -
// Entities 1 and 3 were destroyed during the frame:
// ```
//  std::vector<Entity> entities = {
//      {0, true}, {1, false}, {2, true}, {3, false}, {4, true}};
//
//  const std::size_t count = compact_entities(entities);
//
//  assert(3 == count);  // 'entities' is now 0, 2, 4
// ```
// `main` runs this example:
// ```
//  xmake run 01 a
// ```

// std
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

/// This simple `struct` holds the data of one entity of a game.
struct Entity {
    // DATA

    std::uint32_t d_id;         // unique id of the entity
    bool          d_is_active;  // `false` once the entity is destroyed
    // ... other data (position, velocity, etc.)
};

/// Remove the inactive entities of the specified `entities` and move the
/// active ones to its front, in their original order. Return the number of
/// active entities, which is the new size of `entities`.
[[nodiscard]] std::size_t compact_entities(std::vector<Entity>& entities)
{
    std::size_t wi = 0;
    for (std::size_t i = 0; i < entities.size(); ++i) {
        if (entities[i].d_is_active) {
            if (wi != i) {
                entities[wi] = entities[i];
            }
            ++wi;
        }
    }
    entities.erase(entities.begin() + static_cast<std::ptrdiff_t>(wi),
                   entities.end());
    return wi;
}

}  // close unnamed namespace

int main()
{
    std::vector<Entity> entities = {{0, true},
                                    {1, false},
                                    {2, true},
                                    {3, false},
                                    {4, true}};

    const std::size_t count = compact_entities(entities);

    std::cout << "active entities :";
    for (const Entity& entity : entities) {
        std::cout << ' ' << entity.d_id;
    }
    std::cout << " (" << count << " in total)\n";
}
