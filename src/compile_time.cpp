// COMPILE-TIME PROGRAMMING
// ========================
// Key concepts:
//   1. constexpr: evaluate at compile-time (when possible) or runtime
//   2. consteval (C++20): MUST evaluate at compile-time
//   3. if constexpr: compile-time branching (eliminates dead code paths)
//   4. constexpr classes: objects with constexpr constructors/methods
//   5. Template metaprogramming: recursive template specialization for compile-time computation
//   6. Compile-time lookup tables: constexpr + std::array for zero-cost tables

#include <iostream>
#include <array>
#include <string>
#include <type_traits>

// --- constexpr functions: evaluated at compile-time when arguments are constexpr ---
constexpr int factorial(int n) { return n <= 1 ? 1 : n * factorial(n - 1); }

constexpr int fibonacci(int n) {
    if (n <= 1) return n;
    int a = 0, b = 1;
    for (int i = 2; i <= n; ++i) { int t = a + b; a = b; b = t; }
    return b;
}

// --- if constexpr: compile-time branching (non-matching branches are discarded) ---
template<typename T>
std::string type_name() {
    if constexpr (std::is_integral_v<T>) return "integral";
    else if constexpr (std::is_floating_point_v<T>) return "floating-point";
    else if constexpr (std::is_pointer_v<T>) return "pointer";
    else return "other";
}

// --- constexpr class: methods usable at compile-time ---
class Point {
    double x_, y_;
public:
    constexpr Point(double x, double y) : x_(x), y_(y) {}
    constexpr double distance_sq(const Point& o) const { double dx=x_-o.x_, dy=y_-o.y_; return dx*dx+dy*dy; } // sqrt not constexpr
};

// --- consteval (C++20): MUST be evaluated at compile-time ---
consteval int square(int n) { return n * n; }
consteval bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) { if (n % i == 0) return false; }
    return true;
}

// --- Compile-time lookup table ---
constexpr auto factorial_table = [] {
    std::array<int, 11> t{};
    for (int i = 0, f = 1; i <= 10; ++i) { t[i] = f; f *= (i + 1); }
    return t;
}();

// --- Compile-time string ---
template<size_t N>
struct FixedString {
    char data[N]{};
    constexpr FixedString(const char (&str)[N]) { for (size_t i = 0; i < N; ++i) data[i] = str[i]; }
    constexpr size_t size() const { return N - 1; }
};

// --- Compile-time array algorithms ---
template<typename T, size_t N> constexpr T array_sum(const std::array<T,N>& a) { T t{}; for (size_t i=0;i<N;++i) t+=a[i]; return t; }
template<typename T, size_t N> constexpr T array_max(const std::array<T,N>& a) { T m=a[0]; for (size_t i=1;i<N;++i) if(a[i]>m) m=a[i]; return m; }

// --- Template metaprogramming: recursive compile-time Fibonacci ---
template<int N> struct Fib { static constexpr int value = Fib<N-1>::value + Fib<N-2>::value; };
template<> struct Fib<0> { static constexpr int value = 0; };
template<> struct Fib<1> { static constexpr int value = 1; };

int main() {
    // constexpr functions
    constexpr int f5 = factorial(5);           // compile-time
    int rt = factorial(7);                      // runtime (non-constexpr arg)
    constexpr int fib10 = fibonacci(10);
    std::cout << "factorial(5)=" << f5 << " fibonacci(10)=" << fib10 << "\n";

    // if constexpr
    std::cout << "int:" << type_name<int>() << " double:" << type_name<double>() << " int*:" << type_name<int*>() << "\n";

    // constexpr class
    constexpr Point p1(0,0), p2(3,4);
    constexpr double dist_sq = p1.distance_sq(p2);
    std::cout << "distance²(0,0)→(3,4)=" << dist_sq << "\n";

    // consteval
    constexpr int sq = square(7);
    constexpr bool p11 = is_prime(11);
    std::cout << "square(7)=" << sq << " is_prime(11)=" << std::boolalpha << p11 << "\n";

    // lookup table
    for (int i = 0; i <= 10; ++i) std::cout << i << "!=" << factorial_table[i] << "\n";

    // compile-time string
    constexpr FixedString greeting = "Hello C++20!";
    std::cout << "string size=" << greeting.size() << "\n";

    // array algorithms at compile-time
    constexpr std::array<int,5> arr = {10,20,30,40,50};
    std::cout << "sum=" << array_sum(arr) << " max=" << array_max(arr) << "\n";

    // template metaprogramming
    std::cout << "Fib<10>=" << Fib<10>::value << " Fib<15>=" << Fib<15>::value << "\n";
}
