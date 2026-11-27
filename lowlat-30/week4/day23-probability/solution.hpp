// Day 23 exercise: two optimal-stopping games, solved exactly by dynamic programming.
// Non-template functions defined in this header must be `inline`.
#pragma once

namespace ll::prob {

// Roll a fair six-sided die up to `rolls` times. After each roll you may stop and be paid the face value,
// or roll again (you must take the last roll). Return the expected payout under optimal play. rolls >= 1.
double dice_stop_value(int rolls);

// A deck has `red` red and `black` black cards, shuffled. You draw one at a time: +1 for red, -1 for black.
// You may stop at any time (including before the first draw) and keep the running total.
// Return the expected total under optimal play. red, black >= 0 and at most 52 each.
double red_black_value(int red, int black);

}  // namespace ll::prob
