// 04_cpp20_concepts.cpp
//
// Lesson 4 — C++20 Concepts
//
// Concepts are compile-time predicates that constrain template parameters.
// They provide:
//   - Better error messages (constrained vs unconstrained templates)
//   - Self-documenting code (template requirements are explicit)
//   - Disambiguation of overloads (select best match via concepts)
//   - Subsumption (concept refinement hierarchy)
//
// Patterns shown:
//   A. Defining concepts (Sortable, Container, Hashable)
//   B. Constraining function templates
//   C. Constraining class templates
//   D. Requires expressions
//   E. Subsumption (concept refinement)
//   F. Concepts vs SFINAE comparison
//   G. Built-in concepts

#include <algorithm>
#include <concepts>
#include <deque>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <set>
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
// A. Defining Concepts
// ===========================================================================

template <typename T>
concept Sortable = requires(T a, T b) {
  { a < b } -> std::convertible_to<bool>;
};

template <typename T>
concept Container = requires(T t) {
  typename T::value_type;
  typename T::iterator;
  { t.begin() } -> std::same_as<typename T::iterator>;
  { t.end() } -> std::same_as<typename T::iterator>;
  { t.size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept Hashable = requires(T a) {
  { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept Streamable = requires(std::ostream &os, T a) {
  { os << a } -> std::same_as<std::ostream &>;
};

template <typename T>
concept Numeric = std::is_arithmetic_v<T>;

void demo_defining_concepts() {
  section("A. Defining Concepts");

  std::cout << std::boolalpha;
  std::cout << "  Sortable<int>: " << Sortable<int> << "\n";
  std::cout << "  Sortable<string>: " << Sortable<std::string> << "\n";
  std::cout << "  Container<vector<int>>: "
            << Container<std::vector<int>> << "\n";
  std::cout << "  Container<list<int>>: " << Container<std::list<int>> << "\n";
  std::cout << "  Container<int>: " << Container<int> << "\n";
  std::cout << "  Hashable<int>: " << Hashable<int> << "\n";
  std::cout << "  Hashable<string>: " << Hashable<std::string> << "\n";
  std::cout << "  Streamable<int>: " << Streamable<int> << "\n";
  std::cout << "  Numeric<int>: " << Numeric<int> << "\n";
  std::cout << "  Numeric<double>: " << Numeric<double> << "\n";
  std::cout << "  Numeric<string>: " << Numeric<std::string> << "\n";
}

// ===========================================================================
// B. Constraining Function Templates
// ===========================================================================

template <Sortable T> void print_sorted(std::vector<T> &v) {
  std::sort(v.begin(), v.end());
  std::cout << "  sorted: ";
  for (const auto &x : v)
    std::cout << x << " ";
  std::cout << "\n";
}

template <typename T>
  requires Container<T> && Hashable<typename T::value_type>
void print_container_stats(const T &c) {
  std::cout << "  container size=" << c.size() << "\n";
}

template <Container C>
  requires Numeric<typename C::value_type>
auto sum(const C &c) {
  typename C::value_type result{};
  for (const auto &x : c)
    result += x;
  return result;
}

template <Streamable T> void print(const T &value) { std::cout << value; }

template <Container T> void print(const T &c) {
  std::cout << "[";
  bool first = true;
  for (const auto &x : c) {
    if (!first)
      std::cout << ", ";
    first = false;
    std::cout << x;
  }
  std::cout << "]";
}

void demo_constraining_functions() {
  section("B. Constraining Function Templates");

  std::vector<int> nums = {5, 2, 8, 1, 9, 3};
  print_sorted(nums);

  std::vector<std::string> words = {"cherry", "apple", "banana"};
  print_sorted(words);

  std::vector<int> v = {1, 2, 3, 4, 5};
  print_container_stats(v);

  std::cout << "  sum of {1,2,3,4,5} = " << sum(v) << "\n";

  print(42);
  std::cout << "\n";
  print(std::vector<int>{10, 20, 30});
  std::cout << "\n";
}

// ===========================================================================
// C. Constraining Class Templates
// ===========================================================================

template <Container C> class ContainerPrinter {
public:
  explicit ContainerPrinter(const C &c) : container_(c) {}
  void print_all() const {
    std::cout << "  [";
    bool first = true;
    for (const auto &x : container_) {
      if (!first)
        std::cout << ", ";
      first = false;
      std::cout << x;
    }
    std::cout << "]\n";
  }

private:
  const C &container_;
};

template <Numeric T> class Accumulator {
public:
  void add(T value) {
    total_ += value;
    count_++;
  }
  T total() const { return total_; }
  double average() const {
    return count_ > 0 ? static_cast<double>(total_) / count_ : 0;
  }

private:
  T total_ = 0;
  std::size_t count_ = 0;
};

void demo_constraining_classes() {
  section("C. Constraining Class Templates");

  std::vector<int> v = {10, 20, 30, 40, 50};
  ContainerPrinter printer(v);
  std::cout << "  vector: ";
  printer.print_all();

  std::list<std::string> names = {"Alice", "Bob", "Charlie"};
  ContainerPrinter list_printer(names);
  std::cout << "  list: ";
  list_printer.print_all();

  Accumulator<int> acc;
  acc.add(10);
  acc.add(20);
  acc.add(30);
  std::cout << "  accumulator: total=" << acc.total()
            << " avg=" << acc.average() << "\n";
}

// ===========================================================================
// D. Requires Expressions
// ===========================================================================

template <typename T>
concept HasPushBack =
    requires(T t, typename T::value_type v) { t.push_back(v); };

template <typename T>
concept Addable = requires(T a, T b) {
  { a + b } -> std::convertible_to<T>;
};

template <typename T>
concept EqualityComparable = requires(T a, T b) {
  { a == b } -> std::convertible_to<bool>;
  { a != b } -> std::convertible_to<bool>;
};

template <typename T>
concept ContainerWithPushBack = Container<T> && HasPushBack<T>;

template <ContainerWithPushBack C>
void safe_push_back(C &c, typename C::value_type val) {
  c.push_back(val);
  std::cout << "  pushed back, new size=" << c.size() << "\n";
}

void demo_requires_expressions() {
  section("D. Requires Expressions");

  std::cout << std::boolalpha;
  std::cout << "  HasPushBack<vector<int>>: "
            << HasPushBack<std::vector<int>> << "\n";
  std::cout << "  HasPushBack<set<int>>: "
            << HasPushBack<std::set<int>> << "\n";
  std::cout << "  Addable<int>: " << Addable<int> << "\n";
  std::cout << "  EqualityComparable<string>: "
            << EqualityComparable<std::string> << "\n";

  std::vector<int> v;
  safe_push_back(v, 42);
  safe_push_back(v, 100);
}

// ===========================================================================
// E. Subsumption (Concept Refinement)
// ===========================================================================

template <typename T>
concept BasicContainer = requires(T t) {
  { t.size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept SortedContainer = BasicContainer<T> && requires(T t) {
  { std::is_sorted(t.begin(), t.end()) } -> std::convertible_to<bool>;
};

void print_info(BasicContainer auto const &c) {
  std::cout << "  [BasicContainer] size=" << c.size() << "\n";
}

void print_info(SortedContainer auto const &c) {
  std::cout << "  [SortedContainer] size=" << c.size() << " (sorted!)\n";
}

void demo_subsumption() {
  section("E. Subsumption (Concept Refinement)");

  std::vector<int> unsorted = {5, 2, 8, 1};
  std::vector<int> sorted = {1, 2, 5, 8};

  print_info(unsorted);
  print_info(sorted);

  std::cout << std::boolalpha;
  std::cout << "  BasicContainer<vector<int>>: "
            << BasicContainer<std::vector<int>> << "\n";
  std::cout << "  SortedContainer<vector<int>>: "
            << SortedContainer<std::vector<int>> << "\n";
}

// ===========================================================================
// F. Concepts vs SFINAE Comparison
// ===========================================================================

template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
T sfinae_multiply(T a, T b) {
  return a * b;
}

template <Numeric T> T concept_multiply(T a, T b) { return a * b; }

template <typename T, typename = void>
struct sfinae_has_value_type : std::false_type {};
template <typename T>
struct sfinae_has_value_type<T, std::void_t<typename T::value_type>>
    : std::true_type {};

template <typename T>
concept HasValueType = requires { typename T::value_type; };

template <typename T>
std::enable_if_t<std::is_integral_v<T>, T> sfinae_abs(T x) {
  return x < 0 ? -x : x;
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> sfinae_abs(T x) {
  return std::abs(x);
}

template <typename T>
  requires std::integral<T>
T concept_abs(T x) {
  return x < 0 ? -x : x;
}

template <typename T>
  requires std::floating_point<T>
T concept_abs(T x) {
  return std::abs(x);
}

void demo_sfinae_comparison() {
  section("F. Concepts vs SFINAE Comparison");

  std::cout << "  sfinae_multiply(3, 4) = " << sfinae_multiply(3, 4) << "\n";
  std::cout << "  concept_multiply(3, 4) = " << concept_multiply(3, 4) << "\n";

  std::cout << "  sfinae_has_value_type<vector>: "
            << sfinae_has_value_type<std::vector<int>>::value << "\n";
  std::cout << "  HasValueType<vector>: "
            << HasValueType<std::vector<int>> << "\n";

  std::cout << "  sfinae_abs(-42) = " << sfinae_abs(-42) << "\n";
  std::cout << "  concept_abs(-42) = " << concept_abs(-42) << "\n";
  std::cout << "  concept_abs(-3.14) = " << concept_abs(-3.14) << "\n";

  std::cout << "\n  Key differences:\n";
  std::cout << "  - SFINAE: uses std::enable_if_t, harder to read\n";
  std::cout << "  - Concepts: uses 'requires', self-documenting\n";
  std::cout << "  - SFINAE: error messages refer to substitution failure\n";
  std::cout << "  - Concepts: error messages show which constraint failed\n";
}

// ===========================================================================
// G. Built-in Concepts
// ===========================================================================

void demo_builtin_concepts() {
  section("G. Built-in Concepts (std::concepts)");

  std::cout << std::boolalpha;
  std::cout << "  std::same_as<int, int>: " << std::same_as<int, int> << "\n";
  std::cout << "  std::same_as<int, double>: "
            << std::same_as<int, double> << "\n";
  std::cout << "  std::convertible_to<int, double>: "
            << std::convertible_to<int, double> << "\n";
  std::cout << "  std::integral<int>: " << std::integral<int> << "\n";
  std::cout << "  std::floating_point<double>: "
            << std::floating_point<double> << "\n";
  std::cout << "  std::is_arithmetic_v<int>: "
            << std::is_arithmetic_v<int> << "\n";
  std::cout << "  std::default_initializable<int>: "
            << std::default_initializable<int> << "\n";
  std::cout << "  std::copy_constructible<int>: "
            << std::copy_constructible<int> << "\n";
  std::cout << "  std::equality_comparable<int>: "
            << std::equality_comparable<int> << "\n";
  std::cout << "  std::totally_ordered<int>: "
            << std::totally_ordered<int> << "\n";
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 4: C++20 Concepts ===";

  demo_defining_concepts();
  demo_constraining_functions();
  demo_constraining_classes();
  demo_requires_expressions();
  demo_subsumption();
  demo_sfinae_comparison();
  demo_builtin_concepts();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
