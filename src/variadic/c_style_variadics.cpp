#include <cstdarg>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

int sum_integers(std::size_t count, ...) {
    va_list args;
    va_start(args, count);

    int total = 0;
    for (std::size_t index = 0; index < count; ++index) {
        total += va_arg(args, int);
    }

    va_end(args);
    return total;
}

struct IntegerStats {
    int sum{};
    int max{};
};

IntegerStats analyze_integers(std::size_t count, ...) {
    va_list args;
    va_start(args, count);

    va_list copy;
    va_copy(copy, args);

    int sum = 0;
    for (std::size_t index = 0; index < count; ++index) {
        sum += va_arg(args, int);
    }

    int max = std::numeric_limits<int>::min();
    for (std::size_t index = 0; index < count; ++index) {
        const int current = va_arg(copy, int);
        if (current > max) {
            max = current;
        }
    }

    va_end(copy);
    va_end(args);

    return IntegerStats{sum, max};
}

std::string simple_format(const char* layout, ...) {
    va_list args;
    va_start(args, layout);

    std::ostringstream builder;

    for (const char* token = layout; *token != '\0'; ++token) {
        switch (*token) {
            case 'i':
                builder << "[int:" << va_arg(args, int) << "]";
                break;
            case 'd':
                builder << "[double:" << va_arg(args, double) << "]";
                break;
            case 's':
                builder << "[string:" << va_arg(args, const char*) << "]";
                break;
            default:
                builder << "[unknown-token:" << *token << "]";
                break;
        }
    }

    va_end(args);
    return builder.str();
}

int main() {
    std::cout << "=== C-style variadic arguments ===\n\n";

    std::cout << "1) Fixed-type list\n";
    std::cout << "sum_integers(4, 10, 20, 30, 40) = "
              << sum_integers(4, 10, 20, 30, 40) << "\n\n";

    std::cout << "2) Reading the same argument pack twice with va_copy\n";
    const IntegerStats stats = analyze_integers(5, 7, 4, 9, 2, 1);
    std::cout << "sum = " << stats.sum << ", max = " << stats.max << "\n\n";

    std::cout << "3) Mixed values require an external contract\n";
    std::cout << simple_format("isdi", 42, "hello", 3.5, -7) << "\n\n";

    std::cout << "4) Default promotions matter\n";
    std::cout << "Characters, bool, and short are read back as int.\n";
    std::cout << "float is read back as double.\n";
    std::cout << "Example total = "
              << sum_integers(3, static_cast<int>('A'), static_cast<short>(2), true)
              << "\n\n";

    std::cout << "Rules to remember:\n";
    std::cout << "- The last named parameter is what va_start uses.\n";
    std::cout << "- Every va_start must be matched by va_end.\n";
    std::cout << "- Use va_copy before traversing the list more than once.\n";
    std::cout << "- C-style variadics are runtime-only and not type-safe.\n";
    std::cout << "- Prefer variadic templates when you control the API.\n";
}
