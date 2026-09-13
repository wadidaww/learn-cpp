#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include "math_utils.h"

// Simple test framework
int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    void test_##name(); \
    struct TestRegistrar_##name { \
        TestRegistrar_##name() { \
            std::cout << "  Running " #name "... "; \
            try { \
                test_##name(); \
                tests_passed++; \
                std::cout << "PASSED\n"; \
            } catch (const std::exception& e) { \
                tests_failed++; \
                std::cout << "FAILED: " << e.what() << "\n"; \
            } \
            tests_run++; \
        } \
    } registrar_##name; \
    void test_##name()

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw std::runtime_error( \
        std::string("ASSERT_EQ failed: ") + #a + " != " + #b)

#define ASSERT_NEAR(a, b, eps) \
    if (std::abs((a) - (b)) > (eps)) throw std::runtime_error( \
        std::string("ASSERT_NEAR failed: ") + #a + " !~= " + #b)

#define ASSERT_THROW(expr, ex_type) \
    { bool caught = false; try { expr; } catch (const ex_type&) { caught = true; } \
      if (!caught) throw std::runtime_error("ASSERT_THROW: expected " #ex_type); }

#define ASSERT_TRUE(x) \
    if (!(x)) throw std::runtime_error("ASSERT_TRUE failed: " #x)

#define ASSERT_FALSE(x) \
    if ((x)) throw std::runtime_error("ASSERT_FALSE failed: " #x)

// --- Tests for arithmetic ---
TEST(add_positive) { ASSERT_EQ(math::add(2, 3), 5); }
TEST(add_negative) { ASSERT_EQ(math::add(-1, -1), -2); }
TEST(add_zero) { ASSERT_EQ(math::add(0, 0), 0); }

TEST(subtract_basic) { ASSERT_EQ(math::subtract(5, 3), 2); }
TEST(subtract_negative) { ASSERT_EQ(math::subtract(1, 5), -4); }

TEST(multiply_basic) { ASSERT_EQ(math::multiply(3, 4), 12); }
TEST(multiply_zero) { ASSERT_EQ(math::multiply(5, 0), 0); }

// --- Tests for division ---
TEST(divide_basic) { ASSERT_NEAR(math::divide(10.0, 2.0), 5.0, 1e-9); }
TEST(divide_fractions) { ASSERT_NEAR(math::divide(1.0, 3.0), 0.333333, 1e-5); }
TEST(divide_by_zero) {
    ASSERT_THROW(math::divide(1.0, 0.0), std::invalid_argument);
}

// --- Tests for factorial ---
TEST(factorial_zero) { ASSERT_EQ(math::factorial(0), 1); }
TEST(factorial_one) { ASSERT_EQ(math::factorial(1), 1); }
TEST(factorial_five) { ASSERT_EQ(math::factorial(5), 120); }
TEST(factorial_negative) {
    ASSERT_THROW(math::factorial(-1), std::invalid_argument);
}

// --- Tests for is_prime ---
TEST(prime_2) { ASSERT_TRUE(math::is_prime(2)); }
TEST(prime_7) { ASSERT_TRUE(math::is_prime(7)); }
TEST(not_prime_1) { ASSERT_FALSE(math::is_prime(1)); }
TEST(not_prime_4) { ASSERT_FALSE(math::is_prime(4)); }
TEST(prime_97) { ASSERT_TRUE(math::is_prime(97)); }

// --- Tests for fibonacci ---
TEST(fib_zero_elements) {
    auto result = math::fibonacci(0);
    ASSERT_EQ(static_cast<int>(result.size()), 0);
}
TEST(fib_first_five) {
    auto result = math::fibonacci(5);
    std::vector<int> expected = {0, 1, 1, 2, 3};
    ASSERT_EQ(result.size(), expected.size());
    for (size_t i = 0; i < result.size(); ++i)
        ASSERT_EQ(result[i], expected[i]);
}
TEST(fib_ten) {
    auto result = math::fibonacci(10);
    ASSERT_EQ(result[0], 0);
    ASSERT_EQ(result[1], 1);
    ASSERT_EQ(result[9], 34);
}

// --- Tests for merge_sorted ---
TEST(merge_sorted_basic) {
    std::vector<int> a = {1, 3, 5};
    std::vector<int> b = {2, 4, 6};
    auto result = math::merge_sorted(a, b);
    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    ASSERT_EQ(result.size(), expected.size());
    for (size_t i = 0; i < result.size(); ++i)
        ASSERT_EQ(result[i], expected[i]);
}
TEST(merge_sorted_empty) {
    std::vector<int> a = {1, 2, 3};
    std::vector<int> b;
    auto result = math::merge_sorted(a, b);
    ASSERT_EQ(result.size(), static_cast<size_t>(3));
}

// --- Tests for sum_positive ---
TEST(sum_positive_all) {
    std::vector<int> v = {1, 2, 3};
    ASSERT_EQ(math::sum_positive(v), 6);
}
TEST(sum_positive_mixed) {
    std::vector<int> v = {1, -2, 3, -4, 5};
    ASSERT_EQ(math::sum_positive(v), 9);
}
TEST(sum_positive_none) {
    std::vector<int> v = {-1, -2, -3};
    ASSERT_EQ(math::sum_positive(v), 0);
}

// --- Tests for contains ---
TEST(contains_yes) { ASSERT_TRUE(math::contains({1, 2, 3}, 2)); }
TEST(contains_no) { ASSERT_FALSE(math::contains({1, 2, 3}, 5)); }
TEST(contains_empty) { ASSERT_FALSE(math::contains({}, 1)); }

int main() {
    std::cout << "=== Unit Tests ===\n\n";

    // Tests are auto-registered and run via static initialization
    // by the time we reach main(), all tests have already executed.

    std::cout << "\n=== Results ===\n";
    std::cout << "  Run:     " << tests_run << "\n";
    std::cout << "  Passed:  " << tests_passed << "\n";
    std::cout << "  Failed:  " << tests_failed << "\n";

    if (tests_failed > 0) {
        std::cout << "\n  SOME TESTS FAILED!\n";
        return 1;
    }

    std::cout << "\n  ALL TESTS PASSED!\n";
    return 0;
}
