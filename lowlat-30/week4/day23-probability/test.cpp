#include <cmath>

#include "check.hpp"
#include "solution.hpp"

using namespace ll::prob;

namespace {
bool near(double a, double b, double eps = 1e-9) { return std::fabs(a - b) < eps; }
}  // namespace

TEST(dice_stop_small_cases) {
    CHECK(near(dice_stop_value(1), 3.5));
    CHECK(near(dice_stop_value(2), 4.25));
    CHECK(near(dice_stop_value(3), 14.0 / 3.0));
}

TEST(dice_stop_increases_towards_six) {
    double prev = 0;
    for (int n = 1; n <= 60; ++n) {
        double v = dice_stop_value(n);
        CHECK(v >= prev);
        CHECK(v < 6.0);
        prev = v;
    }
    CHECK(dice_stop_value(60) > 5.9);
}

TEST(red_black_edges) {
    CHECK(near(red_black_value(0, 0), 0));
    CHECK(near(red_black_value(5, 0), 5));
    CHECK(near(red_black_value(0, 7), 0));
    CHECK(near(red_black_value(1, 1), 0.5));
}

TEST(red_black_matches_brute_force_on_small_decks) {
    // Brute force over every arrangement is impossible for 26/26, but for tiny decks we can recurse directly.
    struct B {
        static double v(int r, int b) {
            if (r == 0) return 0;
            if (b == 0) return r;
            double draw = (r * (1 + v(r - 1, b)) + b * (-1 + v(r, b - 1))) / (r + b);
            return draw > 0 ? draw : 0;
        }
    };
    for (int r = 0; r <= 8; ++r)
        for (int b = 0; b <= 8; ++b) CHECK(near(red_black_value(r, b), B::v(r, b)));
}

TEST(red_black_full_deck) {
    double v = red_black_value(26, 26);
    CHECK(v > 2.62 && v < 2.63);
    std::printf("    value of the 26/26 game: %.6f\n", v);
}

int main() { return run_all_tests(); }
