// 07_ranges_views.cpp
//
// Lesson 7 — Ranges and Views (C++20)
//
// Ranges generalize the iterator/pointer pair into a single object.
// Views are lazy range adaptors that compose transformations without
// copying data.
//
// Key concepts:
//   std::ranges::range       — type that has begin() and end()
//   std::ranges::view        — a range that is cheap to copy/move
//   Range adaptor            — pipe-able function that creates a view
//
// Patterns shown:
//   A. std::ranges::range concept
//   B. std::ranges::view_interface (CRTP base)
//   C. Custom range: StepRange
//   D. std::ranges algorithms
//   E. std::views::iota and transform
//   F. View composition with std::ranges

#include <algorithm>
#include <concepts>
#include <iostream>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char *title) {
  std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. std::ranges::range Concept
// ===========================================================================

void demo_range_concept() {
  section("A. std::ranges::range Concept");

  std::cout << std::boolalpha;
  std::cout << "  vector<int>: "
            << std::ranges::range<std::vector<int>> << "\n";
  std::cout << "  int[5]: " << std::ranges::range<int[5]> << "\n";
  std::cout << "  string: " << std::ranges::range<std::string> << "\n";
  std::cout << "  int: " << std::ranges::range<int> << "\n";

  std::vector<int> v = {1, 2, 3, 4, 5};
  std::cout << "  range-for: ";
  for (int x : v)
    std::cout << x << " ";
  std::cout << "\n";
}

// ===========================================================================
// B. std::ranges::view_interface
// ===========================================================================
// view_interface provides size(), empty(), operator[], front(), back()
// for free — you just need begin() and end().
// NOTE: To satisfy forward_range, our iterator must use forward_iterator_tag.

class IotaView : public std::ranges::view_interface<IotaView> {
public:
  IotaView() = default;
  IotaView(int start, int end) : start_(start), end_(end) {}

  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = const int *;
    using reference = const int &;

    iterator() = default;
    explicit iterator(int val) : val_(val) {}

    reference operator*() const { return val_; }
    iterator &operator++() {
      ++val_;
      return *this;
    }
    iterator operator++(int) {
      auto tmp = *this;
      ++val_;
      return tmp;
    }
    bool operator==(const iterator &o) const { return val_ == o.val_; }
    bool operator!=(const iterator &o) const { return !(*this == o); }

  private:
    int val_ = 0;
  };

  iterator begin() const { return iterator(start_); }
  iterator end() const { return iterator(end_); }

private:
  int start_ = 0, end_ = 0;
};

void demo_view_interface() {
  section("B. std::ranges::view_interface");

  IotaView iota(1, 6);

  // view_interface provides empty() via ranges::empty (forward_range)
  std::cout << "  iota(1,6) empty: " << iota.empty() << "\n";
  auto front_val = *std::ranges::begin(iota);
  std::cout << "  iota(1,6) front: " << front_val << "\n";

  // Note: size() requires sized_sentinel_for (operator-), not available
  // for forward iterators. Use std::ranges::distance or count manually.
  auto dist = std::ranges::distance(iota);
  std::cout << "  iota(1,6) distance: " << dist << "\n";

  // back() requires bidirectional_range. Use front() or iterate instead.

  std::cout << "  elements: ";
  for (int x : iota)
    std::cout << x << " ";
  std::cout << "\n";
}

// ===========================================================================
// C. Custom Range: StepRange
// ===========================================================================

class StepRange : public std::ranges::view_interface<StepRange> {
public:
  StepRange(int start, int end, int step = 1)
      : start_(start), end_(end), step_(step) {}

  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = const int *;
    using reference = const int &;

    iterator() = default;
    iterator(int val, int step) : val_(val), step_(step) {}

    reference operator*() const { return val_; }

    iterator &operator++() {
      val_ += step_;
      return *this;
    }
    iterator operator++(int) {
      auto tmp = *this;
      val_ += step_;
      return tmp;
    }

    bool operator==(const iterator &o) const {
      return step_ > 0 ? (val_ >= o.val_) : (val_ <= o.val_);
    }
    bool operator!=(const iterator &o) const { return !(*this == o); }

  private:
    int val_ = 0, step_ = 1;
  };

  iterator begin() const { return iterator(start_, step_); }
  iterator end() const { return iterator(end_, step_); }

private:
  int start_, end_, step_;
};

void demo_step_range() {
  section("C. Custom Range: StepRange");

  std::cout << "  range(0, 20, 3): ";
  for (int x : StepRange(0, 20, 3))
    std::cout << x << " ";
  std::cout << "\n";
}

// ===========================================================================
// D. std::ranges Algorithms
// ===========================================================================

void demo_ranges_algorithms() {
  section("D. std::ranges Algorithms");

  std::vector<int> v = {5, 2, 8, 1, 9, 3, 7, 4, 6};

  std::ranges::sort(v);
  std::cout << "  sorted: ";
  for (int x : v)
    std::cout << x << " ";
  std::cout << "\n";

  auto it = std::ranges::find(v, 4);
  std::cout << "  found 4: " << (it != v.end()) << "\n";

  auto count = std::ranges::count_if(v, [](int x) { return x > 5; });
  std::cout << "  count > 5: " << count << "\n";

  std::cout << "  any > 8: "
            << std::ranges::any_of(v, [](int x) { return x > 8; }) << "\n";
  std::cout << "  all > 0: "
            << std::ranges::all_of(v, [](int x) { return x > 0; }) << "\n";
  std::cout << "  none < 0: "
            << std::ranges::none_of(v, [](int x) { return x < 0; }) << "\n";

  std::cout << "  for_each: ";
  std::ranges::for_each(v, [](int x) { std::cout << x << " "; });
  std::cout << "\n";

  std::vector<int> squared;
  std::ranges::transform(v, std::back_inserter(squared),
                         [](int x) { return x * x; });
  std::cout << "  squared: ";
  for (int x : squared)
    std::cout << x << " ";
  std::cout << "\n";

  auto [min_it, max_it] = std::ranges::minmax_element(v);
  std::cout << "  min=" << *min_it << " max=" << *max_it << "\n";
}

// ===========================================================================
// E. std::views::iota and transform
// ===========================================================================

void demo_views_iota_transform() {
  section("E. std::views::iota and transform");

  // iota: range of integers
  std::cout << "  iota(1, 6): ";
  for (int x : std::views::iota(1, 6))
    std::cout << x << " ";
  std::cout << "\n";

  // transform: apply a function lazily
  std::cout << "  square(iota(1,6)): ";
  for (int x : std::views::iota(1, 6) |
                   std::views::transform([](int x) { return x * x; })) {
    std::cout << x << " ";
  }
  std::cout << "\n";

  // filter + transform
  std::cout << "  even squares: ";
  for (int x : std::views::iota(1, 11) | std::views::filter([](int x) {
                 return x % 2 == 0;
               }) | std::views::transform([](int x) { return x * x; })) {
    std::cout << x << " ";
  }
  std::cout << "\n";
}

// ===========================================================================
// F. View Composition
// ===========================================================================

void demo_view_composition() {
  section("F. View Composition with std::ranges");

  std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  // Compose: filter even, square, take first 3
  auto result = nums | std::views::filter([](int x) { return x % 2 == 0; }) |
                std::views::transform([](int x) { return x * x; }) |
                std::views::take(3);

  std::cout << "  first 3 even squares: ";
  for (int x : result)
    std::cout << x << " ";
  std::cout << "\n";

  // Drop first 2 even, take 3
  auto middle = nums | std::views::filter([](int x) { return x % 2 == 0; }) |
                std::views::drop(2) | std::views::take(3);

  std::cout << "  skip 2 even, take 3: ";
  for (int x : middle)
    std::cout << x << " ";
  std::cout << "\n";

  // Reverse
  std::cout << "  reversed: ";
  for (int x : nums | std::views::reverse)
    std::cout << x << " ";
  std::cout << "\n";

  // enumerate (C++23 — if available)
  // Note: std::views::enumerate requires C++23
  // For C++20, we can use std::views::iota + zip
  std::cout << "  indexed (iota + transform): ";
  for (auto [i, x] : std::views::iota(0, static_cast<int>(nums.size())) |
                         std::views::transform([&nums](int i) {
                           return std::make_pair(i, nums[i]);
                         })) {
    std::cout << "(" << i << ":" << x << ") ";
  }
  std::cout << "\n";
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 7: Ranges and Views (C++20) ===";

  demo_range_concept();
  demo_view_interface();
  demo_step_range();
  demo_ranges_algorithms();
  demo_views_iota_transform();
  demo_view_composition();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
