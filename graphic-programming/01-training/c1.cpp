// c1.cpp                                                             -*-C++-*-

//@PURPOSE: Compare a loop over objects with the same loop over pointers.
//
//@CLASSES:
//  GameObject: one object of a game, 64 bytes
//
//@DESCRIPTION: The same update runs on `k_OBJECTS` game objects reached in
// three ways:
//
//: 1 `std::vector<GameObject>`: the objects sit side by side in memory.
//: 2 `std::vector<GameObject*>`, in allocation order: each object has its own
//:   `new`, but the allocator handed out the blocks one after another, so the
//:   objects are still almost side by side.
//: 3 The same pointers, shuffled: the objects are now scattered, as in a game
//:   where objects are created and destroyed at different times.
//
// 1 and 2 run at about the same speed: the CPU sees that the next object is
// on the next cache line and loads it ahead of time. 3 is several times
// slower: each object is on a cache line nobody asked for, so the CPU waits
// for RAM at almost every object. It is not the pointer that costs, it is
// where it points.
//
///Usage
///-----
// This section illustrates intended use of this component.
//
///Example 1: Measuring the Cost of Scattered Objects
/// - - - - - - - - - - - - - - - - - - - - - - - - -
// Build in release mode (the default), then run:
// ```
//  xmake run graphic 01 c1
// ```

// std
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

namespace {

/// Number of game objects: 64 MB of them, too many to fit in the caches.
constexpr int k_OBJECTS = 1'000'000;

/// Number of runs each printed time is the mean of.
constexpr int k_RUNS = 100;

/// Duration of one frame in seconds, at 60 frames per second.
constexpr float k_DT = 1.0f / 60.0f;

/// This simple `struct` stands for an object of a game: the fields the loop
/// reads and writes (position, velocity), and the rest of the object, which
/// the loop never touches but which still takes up memory. It is 64 bytes:
/// one cache line on x86, half of one on Apple Silicon.
struct GameObject {
    // DATA

    float         d_position[3];  // x, y, z
    float         d_velocity[3];  // x, y, z, in units per second
    float         d_health;       // hit points left
    std::uint32_t d_id;           // unique id of the object
    std::byte     d_other[32];    // the rest: name, flags, components, ...
};

static_assert(sizeof(GameObject) == 64);

/// Move the specified `object` by its velocity over one frame.
void update(GameObject& object)
{
    for (int k = 0; k < 3; ++k) {
        object.d_position[k] += object.d_velocity[k] * k_DT;
    }
}

/// Update each of the specified `objects`.
void update_objects(std::vector<GameObject>& objects)
{
    for (GameObject& object : objects) {
        update(object);
    }
}

/// Update each object the specified `pointers` point to.
void update_pointers(const std::vector<GameObject*>& pointers)
{
    for (GameObject* object : pointers) {
        update(*object);
    }
}

/// Return the mean time, in milliseconds, of `k_RUNS` calls to the specified
/// `update_all`.
template <class t_FUNCTION> double average_time(const t_FUNCTION& update_all)
{
    double total = 0.0;
    for (int run = 0; run < k_RUNS; ++run) {
        const auto start = std::chrono::steady_clock::now();
        update_all();
        const auto end = std::chrono::steady_clock::now();
        const std::chrono::duration<double, std::milli> elapsed = end - start;
        total += elapsed.count();
    }
    return total / k_RUNS;
}

}  // close unnamed namespace

int main()
{
    // Every object starts at the origin and moves along x at 1 unit/s.
    GameObject prototype{};
    prototype.d_velocity[0] = 1.0f;

    // 1. The objects themselves, side by side.
    std::vector<GameObject> objects(k_OBJECTS, prototype);

    // 2. One 'new' per object ('owners' deletes them at the end), and the
    //    pointers to them in allocation order.
    std::vector<std::unique_ptr<GameObject>> owners;
    std::vector<GameObject*>                 pointers;
    owners.reserve(k_OBJECTS);
    pointers.reserve(k_OBJECTS);
    for (int i = 0; i < k_OBJECTS; ++i) {
        owners.push_back(std::make_unique<GameObject>(prototype));
        pointers.push_back(owners.back().get());
    }

    // 3. The same pointers in random order.
    std::vector<GameObject*> shuffled = pointers;
    std::shuffle(shuffled.begin(),
                 shuffled.end(),
                 std::mt19937{std::random_device{}()});

    const double by_value  = average_time([&] { update_objects(objects); });
    const double in_order  = average_time([&] { update_pointers(pointers); });
    const double scattered = average_time([&] { update_pointers(shuffled); });

    std::cout << "objects                    : " << by_value << " ms\n"
              << "pointers, allocation order : " << in_order << " ms\n"
              << "pointers, shuffled         : " << scattered << " ms\n";

    // Print a value that depends on every update: if nothing read the
    // positions, the optimizer could remove the loops we are timing.
    double checksum = 0.0;
    for (const GameObject& object : objects) {
        checksum += object.d_position[0];
    }
    for (const GameObject* object : pointers) {
        checksum += object->d_position[0];
    }
    std::cout << "checksum                   : " << checksum << '\n';
}
