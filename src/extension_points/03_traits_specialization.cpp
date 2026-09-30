// 03_traits_specialization.cpp
//
// Lesson 3 — Traits Specialization
//
// Type traits let you query and customize type properties at compile time.
// The standard library provides many extensible traits that you can
// specialize for your own types:
//
//   - std::hash<T>           -> use your type in unordered containers
//   - std::char_traits<T>    -> custom character type for strings/streams
//   - std::equal_to<T>       -> custom equality comparison
//   - std::less<T>           -> custom ordering
//
// You can also build custom traits from scratch:
//   - Detection idioms (is_iterable, is_addable, etc.)
//   - SFINAE with std::enable_if
//   - std::void_t for expression detection
//
// Patterns shown:
//   A. std::hash specialization
//   B. std::char_traits specialization
//   C. std::equal_to and std::less specialization
//   D. Custom type traits from scratch
//   E. SFINAE with std::enable_if
//   F. Detection idiom with std::void_t

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char *title) {
  std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. std::hash Specialization
// ===========================================================================

struct Point {
  int x, y;
  bool operator==(const Point &o) const { return x == o.x && y == o.y; }
};

struct PointHasher {
  std::size_t operator()(const Point &p) const {
    std::size_t hx = std::hash<int>{}(p.x);
    std::size_t hy = std::hash<int>{}(p.y);
    return hx ^ (hy + 0x9e3779b9 + (hx << 6) + (hx >> 2));
  }
};

template <typename T> inline void hash_combine(std::size_t &seed, const T &v) {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct Rectangle {
  int x, y, width, height;
  bool operator==(const Rectangle &o) const {
    return x == o.x && y == o.y && width == o.width && height == o.height;
  }
};

namespace std {
template <> struct hash<Rectangle> {
  std::size_t operator()(const Rectangle &r) const {
    std::size_t seed = 0;
    hash_combine(seed, r.x);
    hash_combine(seed, r.y);
    hash_combine(seed, r.width);
    hash_combine(seed, r.height);
    return seed;
  }
};
} // namespace std

void demo_hash_specialization() {
  section("A. std::hash Specialization");

  std::unordered_map<Point, std::string, PointHasher> point_names;
  point_names[{1, 2}] = "origin_shifted";
  point_names[{0, 0}] = "origin";

  std::cout << "  point_names:\n";
  for (const auto &[pt, name] : point_names) {
    std::cout << "    (" << pt.x << "," << pt.y << ") -> " << name << "\n";
  }

  std::unordered_map<Rectangle, std::string> rect_map;
  rect_map[{0, 0, 10, 10}] = "small_square";
  rect_map[{0, 0, 100, 100}] = "large_square";

  std::cout << "  rect_map:\n";
  for (const auto &[rect, name] : rect_map) {
    std::cout << "    [" << rect.x << "," << rect.y << " " << rect.width << "x"
              << rect.height << "] -> " << name << "\n";
  }
}

// ===========================================================================
// B. std::char_traits Specialization
// ===========================================================================
// A case-insensitive character type for demonstration.

// Must be trivial for use with std::basic_string
struct CaseInsensitiveChar {
  char value;

  char to_lower() const {
    return (value >= 'A' && value <= 'Z') ? static_cast<char>(value + 32)
                                          : value;
  }
};

namespace std {
template <> struct char_traits<CaseInsensitiveChar> {
  using char_type = CaseInsensitiveChar;
  using int_type = int;
  using off_type = std::streamoff;
  using pos_type = std::streampos;
  using state_type = std::mbstate_t;

  static void assign(char_type &a, const char_type &b) { a.value = b.value; }
  static bool eq(const char_type &a, const char_type &b) {
    return a.to_lower() == b.to_lower();
  }
  static bool lt(const char_type &a, const char_type &b) {
    return a.to_lower() < b.to_lower();
  }
  static int compare(const char_type *s1, const char_type *s2, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      if (eq(s1[i], s2[i]))
        continue;
      if (lt(s1[i], s2[i]))
        return -1;
      return 1;
    }
    return 0;
  }
  static std::size_t length(const char_type *s) {
    std::size_t len = 0;
    while (s[len].value != '\0')
      ++len;
    return len;
  }
  static const char_type *find(const char_type *s, std::size_t n,
                               const char_type &a) {
    for (std::size_t i = 0; i < n; ++i)
      if (eq(s[i], a))
        return s + i;
    return nullptr;
  }
  static char_type *move(char_type *s1, const char_type *s2, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i)
      assign(s1[i], s2[i]);
    return s1;
  }
  static char_type *copy(char_type *s1, const char_type *s2, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i)
      assign(s1[i], s2[i]);
    return s1;
  }
  static char_type *assign(char_type *s, std::size_t n, char_type a) {
    for (std::size_t i = 0; i < n; ++i)
      assign(s[i], a);
    return s;
  }
  static int_type not_eof(const int_type &c) { return c; }
  static char_type to_char_type(const int_type &c) {
    return char_type{static_cast<char>(c)};
  }
  static int_type to_int_type(const char_type &c) {
    return static_cast<int>(c.value);
  }
  static bool eq_int_type(const int_type &a, const int_type &b) {
    return a == b;
  }
  static int_type eof() { return -1; }
};
} // namespace std

using CaseInsensitiveString = std::basic_string<CaseInsensitiveChar>;

std::ostream &operator<<(std::ostream &os, const CaseInsensitiveString &s) {
  for (const auto &c : s)
    os << c.value;
  return os;
}

// Helper to build CaseInsensitiveString from a C string
CaseInsensitiveString make_ci_string(const char *str) {
  CaseInsensitiveString result;
  for (; *str; ++str) {
    CaseInsensitiveChar c{*str};
    result += c;
  }
  return result;
}

void demo_char_traits() {
  section("B. std::char_traits Specialization");

  auto hello = make_ci_string("Hello");
  auto world = make_ci_string("hello");

  std::cout << "  hello=\"" << hello << "\", world=\"" << world << "\"\n";
  std::cout << "  hello == world (case-insensitive): " << (hello == world)
            << "\n";

  auto s = make_ci_string("HeLLo WoRLd");
  std::cout << "  original: " << s << "\n";

  auto pos = s.find(CaseInsensitiveChar{'l'});
  std::cout << "  find('l'): position=" << pos << "\n";
}

// ===========================================================================
// C. std::equal_to and std::less Specialization
// ===========================================================================

struct Temperature {
  double celsius;
};

namespace std {
template <> struct equal_to<Temperature> {
  bool operator()(const Temperature &a, const Temperature &b) const {
    return std::abs(a.celsius - b.celsius) < 0.001;
  }
};

template <> struct less<Temperature> {
  bool operator()(const Temperature &a, const Temperature &b) const {
    return a.celsius < b.celsius;
  }
};
} // namespace std

void demo_comparison_specialization() {
  section("C. std::equal_to and std::less Specialization");

  Temperature t1{20.0}, t2{20.0001}, t3{25.0};

  std::cout << std::boolalpha;
  std::cout << "  t1 == t2 (tolerance): "
            << std::equal_to<Temperature>{}(t1, t2) << "\n";
  std::cout << "  t1 < t3: " << std::less<Temperature>{}(t1, t3) << "\n";

  std::vector<Temperature> temps = {{20.0}, {25.0}, {20.0001}, {30.0}};
  auto it = std::find_if(temps.begin(), temps.end(), [](const Temperature &t) {
    return std::equal_to<Temperature>{}(t, Temperature{25.0});
  });
  if (it != temps.end()) {
    std::cout << "  found temperature: " << it->celsius << "\n";
  }
}

// ===========================================================================
// D. Custom Type Traits from Scratch
// ===========================================================================

template <typename T, typename = void> struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()),
                                  decltype(std::declval<T>().end())>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_iterable_v = is_iterable<T>::value;

template <typename T, typename U, typename = void>
struct is_addable : std::false_type {};

template <typename T, typename U>
struct is_addable<T, U,
                  std::void_t<decltype(std::declval<T>() + std::declval<U>())>>
    : std::true_type {};

template <typename T, typename U>
inline constexpr bool is_addable_v = is_addable<T, U>::value;

template <typename T, typename = void> struct is_hashable : std::false_type {};

template <typename T>
struct is_hashable<T, std::void_t<decltype(std::hash<T>{}(std::declval<T>()))>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_hashable_v = is_hashable<T>::value;

template <typename T, typename = void>
struct is_streamable : std::false_type {};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream &>()
                                             << std::declval<T>())>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_streamable_v = is_streamable<T>::value;

void demo_custom_traits() {
  section("D. Custom Type Traits from Scratch");

  std::cout << std::boolalpha;
  std::cout << "  is_iterable<vector<int>>: "
            << is_iterable_v<std::vector<int>> << "\n";
  std::cout << "  is_iterable<int>: " << is_iterable_v<int> << "\n";
  std::cout << "  is_addable<int, double>: "
            << is_addable_v<int, double> << "\n";
  std::cout << "  is_hashable<int>: " << is_hashable_v<int> << "\n";
  std::cout << "  is_hashable<Point>: " << is_hashable_v<Point> << "\n";
  std::cout << "  is_streamable<int>: " << is_streamable_v<int> << "\n";
}

// ===========================================================================
// E. SFINAE with std::enable_if
// ===========================================================================

template <typename T>
std::enable_if_t<std::is_integral_v<T>, T> safe_divide(T a, T b) {
  if (b == 0)
    throw std::runtime_error("division by zero");
  return a / b;
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> safe_divide(T a, T b) {
  return a / b;
}

template <typename T>
std::enable_if_t<is_iterable_v<T>, std::size_t> container_size(const T &c) {
  return c.size();
}

template <typename T, std::size_t N> std::size_t container_size(T (&)[N]) {
  return N;
}

void demo_sfinae() {
  section("E. SFINAE with std::enable_if");

  std::cout << "  safe_divide(10, 3) = " << safe_divide(10, 3) << "\n";
  std::cout << "  safe_divide(10.0, 3.0) = " << safe_divide(10.0, 3.0) << "\n";

  std::vector<int> v = {1, 2, 3};
  int arr[] = {4, 5, 6, 7};
  std::cout << "  container_size(vector) = " << container_size(v) << "\n";
  std::cout << "  container_size(array) = " << container_size(arr) << "\n";
}

// ===========================================================================
// F. Detection Idiom with std::void_t
// ===========================================================================

template <typename T, typename = void> struct has_size : std::false_type {};

template <typename T>
struct has_size<T, std::void_t<decltype(std::declval<T>().size())>>
    : std::true_type {};

template <typename T> inline constexpr bool has_size_v = has_size<T>::value;

template <typename T, typename = void> struct has_clear : std::false_type {};

template <typename T>
struct has_clear<T, std::void_t<decltype(std::declval<T>().clear())>>
    : std::true_type {};

template <typename T> inline constexpr bool has_clear_v = has_clear<T>::value;

template <typename T>
std::enable_if_t<has_clear_v<T>> clear_if_possible(T &container) {
  container.clear();
  std::cout << "  cleared container\n";
}

template <typename T>
std::enable_if_t<!has_clear_v<T>> clear_if_possible(const T &) {
  std::cout << "  type has no clear() — nothing to do\n";
}

void demo_void_t() {
  section("F. Detection Idiom with std::void_t");

  std::cout << std::boolalpha;
  std::cout << "  has_size<vector>: " << has_size_v<std::vector<int>> << "\n";
  std::cout << "  has_size<int>: " << has_size_v<int> << "\n";
  std::cout << "  has_clear<vector>: " << has_clear_v<std::vector<int>> << "\n";
  std::cout << "  has_clear<int>: " << has_clear_v<int> << "\n";

  std::vector<int> v = {1, 2, 3};
  clear_if_possible(v);
  std::cout << "  vector size after clear: " << v.size() << "\n";

  int x = 42;
  clear_if_possible(x);
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 3: Traits Specialization ===";

  demo_hash_specialization();
  demo_char_traits();
  demo_comparison_specialization();
  demo_custom_traits();
  demo_sfinae();
  demo_void_t();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
