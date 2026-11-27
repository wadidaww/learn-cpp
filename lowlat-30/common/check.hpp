// Minimal test harness so every exercise builds with no dependencies.
// Usage:
//   TEST(name) { CHECK(x == 1); CHECK_EQ(a, b); }
//   int main() { return run_all_tests(); }
#pragma once

#include <cstdio>
#include <functional>
#include <vector>

namespace ll::test {

struct Case {
    const char* name;
    void (*fn)();
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failures() {
    static int n = 0;
    return n;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

}  // namespace ll::test

#define LL_CONCAT2(a, b) a##b
#define LL_CONCAT(a, b) LL_CONCAT2(a, b)

#define TEST(name)                                                                \
    static void LL_CONCAT(test_, name)();                                         \
    static ::ll::test::Registrar LL_CONCAT(reg_, name)(#name, &LL_CONCAT(test_, name)); \
    static void LL_CONCAT(test_, name)()

#define CHECK(cond)                                                               \
    do {                                                                          \
        if (!(cond)) {                                                            \
            std::printf("    FAIL %s:%d  CHECK(%s)\n", __FILE__, __LINE__, #cond); \
            ++::ll::test::failures();                                             \
        }                                                                         \
    } while (0)

#define CHECK_EQ(a, b)                                                                     \
    do {                                                                                   \
        if (!((a) == (b))) {                                                               \
            std::printf("    FAIL %s:%d  CHECK_EQ(%s, %s)\n", __FILE__, __LINE__, #a, #b); \
            ++::ll::test::failures();                                                      \
        }                                                                                  \
    } while (0)

inline int run_all_tests() {
    int failed_cases = 0;
    for (auto& c : ::ll::test::registry()) {
        int before = ::ll::test::failures();
        c.fn();
        bool ok = ::ll::test::failures() == before;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", c.name);
        failed_cases += ok ? 0 : 1;
    }
    std::printf("%zu tests, %d failed\n", ::ll::test::registry().size(), failed_cases);
    return failed_cases == 0 ? 0 : 1;
}
