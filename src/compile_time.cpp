#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <utility>
#include <cmath>

// --- 1. constexpr functions ---
constexpr int factorial(int n) {
    if (n <= 1) return 1;
    int result = 1;
    for (int i = 2; i <= n; ++i) result *= i;
    return result;
}

constexpr int fibonacci(int n) {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) {
        int tmp = a + b;
        a = b;
        b = tmp;
    }
    return b;
}

// --- 2. constexpr with if constexpr ---
template<typename T>
std::string type_name() {
    if constexpr (std::is_integral_v<T>) return "integral";
    else if constexpr (std::is_floating_point_v<T>) return "floating-point";
    else if constexpr (std::is_pointer_v<T>) return "pointer";
    else return "other";
}

// --- 3. constexpr class ---
class Point {
    double x_, y_;
public:
    constexpr Point(double x, double y) : x_(x), y_(y) {}
    constexpr double x() const { return x_; }
    constexpr double y() const { return y_; }
    constexpr double distance_to(const Point& other) const {
        double dx = x_ - other.x_;
        double dy = y_ - other.y_;
        return std::sqrt(dx * dx + dy * dy);
    }
};

// --- 4. consteval (C++20) ---
consteval int square(int n) {
    return n * n;
}

consteval bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

// --- 5. Compile-time lookup table ---
constexpr auto make_factorial_table() {
    std::array<int, 11> table{};
    for (int i = 0; i <= 10; ++i) {
        int result = 1;
        for (int j = 2; j <= i; ++j) result *= j;
        table[i] = result;
    }
    return table;
}
constexpr auto factorial_table = make_factorial_table();

// --- 6. Compile-time string ---
template<size_t N>
struct FixedString {
    char data[N]{};
    constexpr FixedString(const char (&str)[N]) {
        for (size_t i = 0; i < N; ++i) data[i] = str[i];
    }
    constexpr size_t size() const { return N - 1; }
    constexpr char operator[](size_t i) const { return data[i]; }
};

// --- 7. Template metaprogramming with constexpr ---
template<typename T, size_t N>
constexpr T array_sum(const std::array<T, N>& arr) {
    T total{};
    for (size_t i = 0; i < N; ++i) total += arr[i];
    return total;
}

template<typename T, size_t N>
constexpr T array_max(const std::array<T, N>& arr) {
    T max_val = arr[0];
    for (size_t i = 1; i < N; ++i) {
        if (arr[i] > max_val) max_val = arr[i];
    }
    return max_val;
}

// --- 8. constexpr bit operations ---
constexpr int count_set_bits(unsigned int n) {
    int count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

constexpr unsigned int log2_floor(unsigned int n) {
    unsigned int result = 0;
    while (n > 1) {
        n >>= 1;
        ++result;
    }
    return result;
}

// --- 9. SFINAE with constexpr ---
template<typename T>
constexpr auto safe_divide(T a, T b) -> std::enable_if_t<std::is_floating_point_v<T>, T> {
    if (b == T{}) return T{};
    return a / b;
}

// --- 10. Compile-time recursion ---
template<int N>
struct Fibonacci {
    static constexpr int value = Fibonacci<N-1>::value + Fibonacci<N-2>::value;
};
template<> struct Fibonacci<0> { static constexpr int value = 0; };
template<> struct Fibonacci<1> { static constexpr int value = 1; };

int main() {
    // --- 1. constexpr functions ---
    std::cout << "=== 1. constexpr Functions ===\n";
    constexpr int f5 = factorial(5);
    std::cout << "  factorial(5) = " << f5 << " (compile-time)\n";

    int runtime_n = 7;
    std::cout << "  factorial(7) = " << factorial(runtime_n) << " (runtime)\n";

    constexpr int fib10 = fibonacci(10);
    std::cout << "  fibonacci(10) = " << fib10 << "\n";

    // --- 2. if constexpr ---
    std::cout << "\n=== 2. if constexpr ===\n";
    std::cout << "  int: " << type_name<int>() << "\n";
    std::cout << "  double: " << type_name<double>() << "\n";
    std::cout << "  int*: " << type_name<int*>() << "\n";
    std::cout << "  std::string: " << type_name<std::string>() << "\n";

    // --- 3. constexpr class ---
    std::cout << "\n=== 3. constexpr Class ===\n";
    constexpr Point p1(0.0, 0.0);
    constexpr Point p2(3.0, 4.0);
    constexpr double dist = p1.distance_to(p2);
    std::cout << "  distance(0,0)→(3,4) = " << dist << " (compile-time)\n";

    // --- 4. consteval ---
    std::cout << "\n=== 4. consteval (C++20) ===\n";
    constexpr int sq = square(7);
    std::cout << "  square(7) = " << sq << "\n";

    constexpr bool p11 = is_prime(11);
    constexpr bool p12 = is_prime(12);
    std::cout << "  is_prime(11) = " << std::boolalpha << p11 << "\n";
    std::cout << "  is_prime(12) = " << p12 << "\n";

    // --- 5. Lookup table ---
    std::cout << "\n=== 5. Compile-time Lookup Table ===\n";
    for (int i = 0; i <= 10; ++i)
        std::cout << "  " << i << "! = " << factorial_table[i] << "\n";

    // --- 6. Compile-time string ---
    std::cout << "\n=== 6. Compile-time String ===\n";
    constexpr FixedString greeting = "Hello, C++20!";
    std::cout << "  size: " << greeting.size() << "\n";
    std::cout << "  [0]: '" << greeting[0] << "'\n";

    // --- 7. Array algorithms at compile time ---
    std::cout << "\n=== 7. Compile-time Array Algorithms ===\n";
    constexpr std::array<int, 5> arr = {10, 20, 30, 40, 50};
    constexpr int sum = array_sum(arr);
    constexpr int max = array_max(arr);
    std::cout << "  sum = " << sum << "\n";
    std::cout << "  max = " << max << "\n";

    // --- 8. Bit operations ---
    std::cout << "\n=== 8. Compile-time Bit Operations ===\n";
    constexpr unsigned int val = 0b10110101;
    constexpr int bits = count_set_bits(val);
    constexpr unsigned int lg2 = log2_floor(val);
    std::cout << "  " << val << " has " << bits << " set bits\n";
    std::cout << "  log2(" << val << ") = " << lg2 << "\n";

    // --- 9. Template metaprogramming ---
    std::cout << "\n=== 9. Template Metaprogramming ===\n";
    std::cout << "  Fibonacci<10> = " << Fibonacci<10>::value << "\n";
    std::cout << "  Fibonacci<15> = " << Fibonacci<15>::value << "\n";

    // --- 10. Compile-time vs runtime ---
    std::cout << "\n=== 10. Compile-time vs Runtime ===\n";
    // This must be compile-time:
    constexpr int ct = factorial(10);
    std::cout << "  compile-time factorial(10) = " << ct << "\n";
    // This is runtime (variable is not constexpr):
    int rt = factorial(10);
    std::cout << "  runtime factorial(10)     = " << rt << "\n";

    std::cout << "\nDone.\n";
    return 0;
}
