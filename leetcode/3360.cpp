#include <cassert>
#include <iostream>

namespace {

constexpr int k_DEFAULT_INITIAL_STONES = 10;

/// Returns whether the first player (Alice) wins the stone removal game.
///
/// Turn 1 removes `initial_stones`; each subsequent turn removes one fewer
/// stone than the previous turn. The first player unable to make a full move
/// loses.
///
/// Preconditions:
///   - `remaining_stones >= 0`
///   - `initial_stones > 0`
///
/// Complexity: O(initial_stones) time (O(1) for fixed `initial_stones`),
/// O(1) space.
[[nodiscard]] constexpr bool
can_alice_win(int       remaining_stones,
              const int initial_stones = k_DEFAULT_INITIAL_STONES) noexcept
{
    assert(remaining_stones >= 0);
    assert(initial_stones > 0);

    int stones_to_remove = initial_stones;

    while (stones_to_remove > 0 && remaining_stones >= stones_to_remove) {
        remaining_stones -= stones_to_remove;
        --stones_to_remove;
    }

    const int turns_played = initial_stones - stones_to_remove;
    return (turns_played % 2) != 0;
}

static_assert(!can_alice_win(0));      // 0 turns
static_assert(!can_alice_win(1));      // LeetCode minimum
static_assert(!can_alice_win(9));      // just below turn 1
static_assert(can_alice_win(10));      // turn 1 (Alice)
static_assert(can_alice_win(18));      // still turn 1
static_assert(!can_alice_win(19));     // turn 2 (Bob)
static_assert(can_alice_win(27));      // turn 3 (Alice)
static_assert(!can_alice_win(34));     // turn 4 (Bob)
static_assert(can_alice_win(40));      // turn 5 (Alice)
static_assert(!can_alice_win(45));     // turn 6 (Bob)
static_assert(can_alice_win(49));      // turn 7 (Alice)
static_assert(can_alice_win(50));      // LeetCode maximum
static_assert(!can_alice_win(52));     // turn 8 (Bob)
static_assert(can_alice_win(54));      // turn 9 (Alice)
static_assert(!can_alice_win(55));     // turn 10 (all stones exhausted)
static_assert(!can_alice_win(100));    // > 55, terminates cleanly
static_assert(!can_alice_win(0, 9));   // odd initial_stones, 0 turns
static_assert(can_alice_win(9, 9));    // odd initial_stones, 1 turn
static_assert(!can_alice_win(17, 9));  // odd initial_stones, 2 turns

}  // namespace

int main()
{
    std::cout << std::boolalpha << "can_alice_win(12) = " << can_alice_win(12)
              << '\n'
              << "can_alice_win(1)  = " << can_alice_win(1) << '\n';
    return 0;
}
