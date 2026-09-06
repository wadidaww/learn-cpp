// 02_custom_iterators.cpp
//
// Lesson 2 — Custom Iterators
//
// Iterators are the bridge between algorithms and data structures.  By
// implementing iterators you can:
//
//   - Make your custom type iterable with range-for
//   - Use STL algorithms on your data
//   - Create lazy generators (fibonacci, primes, etc.)
//   - Build filtering pipelines
//
// Iterator categories (weakest -> strongest):
//   input_iterator       -> single-pass, read-only
//   forward_iterator     -> multi-pass, read-only
//   bidirectional_iterator -> multi-pass, read/write, can go backwards
//   random_access_iterator -> multi-pass, O(1) jump, comparable
//
// Patterns shown:
//   A. Iterator category tags
//   B. std::iterator_traits specialization
//   C. Fibonacci generator (input iterator)
//   D. Filtered range wrapper
//   E. Tree iterator (bidirectional)
//   F. Sentinel iterators (C++20)
//   G. Making custom type a range

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char *title) {
  std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. Iterator Category Tags
// ===========================================================================

class CounterIterator {
public:
  using iterator_category = std::input_iterator_tag;
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using pointer = const int *;
  using reference = const int &;

  explicit CounterIterator(int start = 0) : value_(start) {}

  reference operator*() const { return value_; }
  pointer operator->() const { return &value_; }

  CounterIterator &operator++() {
    ++value_;
    return *this;
  }
  CounterIterator operator++(int) {
    auto tmp = *this;
    ++value_;
    return tmp;
  }

  bool operator==(const CounterIterator &other) const {
    return value_ == other.value_;
  }
  bool operator!=(const CounterIterator &other) const {
    return !(*this == other);
  }

private:
  int value_;
};

void demo_category_tags() {
  section("A. Iterator Category Tags");

  using category = std::iterator_traits<CounterIterator>::iterator_category;
  std::cout << "  CounterIterator category: " << typeid(category).name()
            << "\n";

  std::cout << "  is input_iterator: "
            << std::is_convertible_v<category, std::input_iterator_tag> << "\n";
  std::cout
      << "  is forward_iterator: "
      << std::is_convertible_v<category, std::forward_iterator_tag> << "\n";

  CounterIterator begin(1), end(6);
  int sum = 0;
  for (auto it = begin; it != end; ++it)
    sum += *it;
  std::cout << "  sum of 1..5 = " << sum << "\n";
}

// ===========================================================================
// B. std::iterator_traits Specialization
// ===========================================================================

template <typename T> class RangeGenerator {
public:
  using value_type = T;
  using difference_type = std::ptrdiff_t;

  RangeGenerator(T start, T end, T step = T(1))
      : current_(start), end_(end), step_(step) {}

  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = const T *;
    using reference = const T &;

    iterator() = default;
    iterator(T val, T end, T step) : val_(val), end_(end), step_(step) {}

    reference operator*() const { return val_; }
    pointer operator->() const { return &val_; }

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
      return (step_ > 0) ? (val_ >= o.val_) : (val_ <= o.val_);
    }
    bool operator!=(const iterator &o) const { return !(*this == o); }

  private:
    T val_{}, end_{}, step_{};
  };

  iterator begin() const { return iterator(current_, end_, step_); }
  iterator end() const { return iterator(end_, end_, step_); }

private:
  T current_, end_, step_;
};

void demo_iterator_traits() {
  section("B. std::iterator_traits Specialization");

  RangeGenerator<double> gen(0.0, 1.0, 0.25);
  std::cout << "  range [0.0, 1.0) step 0.25: ";
  for (double v : gen)
    std::cout << v << " ";
  std::cout << "\n";

  auto dist = std::distance(gen.begin(), gen.end());
  std::cout << "  distance = " << dist << "\n";
}

// ===========================================================================
// C. Fibonacci Generator (Input Iterator)
// ===========================================================================

class FibonacciIterator {
public:
  using iterator_category = std::input_iterator_tag;
  using value_type = unsigned long long;
  using difference_type = std::ptrdiff_t;
  using pointer = const value_type *;
  using reference = const value_type &;

  FibonacciIterator() : a_(0), b_(0), end_(true) {}
  explicit FibonacciIterator(int /*unused*/) : a_(0), b_(1), end_(false) {}

  reference operator*() const { return a_; }
  pointer operator->() const { return &a_; }

  FibonacciIterator &operator++() {
    auto next = a_ + b_;
    a_ = b_;
    b_ = next;
    return *this;
  }
  FibonacciIterator operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  bool operator==(const FibonacciIterator &o) const { return end_ == o.end_; }
  bool operator!=(const FibonacciIterator &o) const { return !(*this == o); }

private:
  value_type a_, b_;
  bool end_;
};

class FibonacciRange {
public:
  explicit FibonacciRange(int count) : count_(count) {}
  FibonacciIterator begin() const { return FibonacciIterator(0); }
  FibonacciIterator end() const { return FibonacciIterator(); }
  int size() const { return count_; }

private:
  int count_;
};

void demo_fibonacci_generator() {
  section("C. Fibonacci Generator — input iterator");

  std::cout << "  first 15 Fibonacci numbers: ";
  int n = 0;
  for (auto val : FibonacciRange(15)) {
    if (n++ >= 15)
      break;
    std::cout << val << " ";
  }
  std::cout << "\n";

  FibonacciIterator begin(0), end;
  auto it =
      std::find_if(begin, end, [](unsigned long long v) { return v > 100; });
  std::cout << "  first Fibonacci > 100: " << *it << "\n";
}

// ===========================================================================
// D. Filtered Range Wrapper
// ===========================================================================

template <typename Container, typename Predicate> class FilteredRange {
public:
  using BaseIterator = typename Container::iterator;
  using ValueType = typename Container::value_type;

  FilteredRange(Container &c, Predicate pred)
      : container_(c), pred_(std::move(pred)) {}

  class iterator {
  public:
    using iterator_category = std::input_iterator_tag;
    using IterValueType = ValueType;
    using difference_type = std::ptrdiff_t;

    iterator() = default;
    iterator(BaseIterator it, BaseIterator end, const Predicate *pred)
        : it_(it), end_(end), pred_(pred) {
      advance_to_valid();
    }

    ValueType &operator*() { return *it_; }
    const ValueType &operator*() const { return *it_; }

    iterator &operator++() {
      ++it_;
      advance_to_valid();
      return *this;
    }
    iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const iterator &o) const { return it_ == o.it_; }
    bool operator!=(const iterator &o) const { return !(*this == o); }

  private:
    void advance_to_valid() {
      while (it_ != end_ && !(*pred_)(*it_))
        ++it_;
    }
    BaseIterator it_, end_;
    const Predicate *pred_ = nullptr;
  };

  iterator begin() {
    return iterator(container_.begin(), container_.end(), &pred_);
  }
  iterator end() {
    return iterator(container_.end(), container_.end(), &pred_);
  }

private:
  Container &container_;
  Predicate pred_;
};

template <typename Container, typename Predicate>
FilteredRange<Container, Predicate> filter(Container &c, Predicate pred) {
  return FilteredRange<Container, Predicate>(c, std::move(pred));
}

void demo_filtered_range() {
  section("D. Filtered Range Wrapper");

  std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  std::cout << "  even numbers: ";
  for (const auto &v : filter(nums, [](int x) { return x % 2 == 0; })) {
    std::cout << v << " ";
  }
  std::cout << "\n";

  std::cout << "  numbers > 5: ";
  for (const auto &v : filter(nums, [](int x) { return x > 5; })) {
    std::cout << v << " ";
  }
  std::cout << "\n";
}

// ===========================================================================
// E. Tree Iterator (Bidirectional) — using flat array representation
// ===========================================================================
// Using a flat array avoids the dangling parent pointer issue with
// unique_ptr tree nodes.  We store indices instead of pointers.

struct TreeNode {
  int value;
  int left = -1; // index into tree array, -1 = none
  int right = -1;
  int parent = -1;
};

class BinaryTree {
public:
  void insert(int val) {
    int idx = static_cast<int>(nodes_.size());
    nodes_.push_back({val});

    if (root_ == -1) {
      root_ = 0;
      return;
    }

    int curr = root_;
    int par = -1;
    while (curr != -1) {
      par = curr;
      if (val < nodes_[curr].value) {
        curr = nodes_[curr].left;
      } else {
        curr = nodes_[curr].right;
      }
    }

    nodes_[idx].parent = par;
    if (val < nodes_[par].value)
      nodes_[par].left = idx;
    else
      nodes_[par].right = idx;
  }

  class iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = const int *;
    using reference = const int &;

    iterator() : nodes_(nullptr), idx_(-1), root_(-1) {}
    iterator(const std::vector<TreeNode> *nodes, int idx, int root = -1)
        : nodes_(nodes), idx_(idx), root_(root) {}

    reference operator*() const { return (*nodes_)[idx_].value; }

    iterator &operator++() {
      idx_ = next_inorder(idx_);
      return *this;
    }
    iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    iterator &operator--() {
      if (idx_ == -1) {
        idx_ = rightmost_in_tree(root_);
      } else {
        idx_ = prev_inorder(idx_);
      }
      return *this;
    }
    iterator operator--(int) {
      auto tmp = *this;
      --(*this);
      return tmp;
    }

    bool operator==(const iterator &o) const { return idx_ == o.idx_; }
    bool operator!=(const iterator &o) const { return !(*this == o); }

  private:
    const std::vector<TreeNode> *nodes_;
    int idx_;
    int root_;

    int next_inorder(int n) const {
      if (n == -1)
        return -1;
      if ((*nodes_)[n].right != -1) {
        int r = (*nodes_)[n].right;
        while ((*nodes_)[r].left != -1)
          r = (*nodes_)[r].left;
        return r;
      }
      int p = (*nodes_)[n].parent;
      while (p != -1 && n == (*nodes_)[p].right) {
        n = p;
        p = (*nodes_)[p].parent;
      }
      return p;
    }

    int prev_inorder(int n) const {
      if (n == -1)
        return -1;
      if ((*nodes_)[n].left != -1) {
        int l = (*nodes_)[n].left;
        while ((*nodes_)[l].right != -1)
          l = (*nodes_)[l].right;
        return l;
      }
      int p = (*nodes_)[n].parent;
      while (p != -1 && n == (*nodes_)[p].left) {
        n = p;
        p = (*nodes_)[p].parent;
      }
      return p;
    }

    int rightmost_in_tree(int n) const {
      if (n == -1)
        return -1;
      while ((*nodes_)[n].right != -1)
        n = (*nodes_)[n].right;
      return n;
    }
  };

  iterator begin() { return iterator(&nodes_, leftmost(root_), root_); }
  iterator end() { return iterator(&nodes_, -1, root_); }

private:
  std::vector<TreeNode> nodes_;
  int root_ = -1;

  int leftmost(int n) const {
    if (n == -1)
      return -1;
    while (nodes_[n].left != -1)
      n = nodes_[n].left;
    return n;
  }
};

void demo_tree_iterator() {
  section("E. Tree Iterator — bidirectional in-order traversal");

  BinaryTree tree;
  tree.insert(5);
  tree.insert(3);
  tree.insert(7);
  tree.insert(1);
  tree.insert(4);
  tree.insert(6);
  tree.insert(8);

  std::cout << "  in-order (forward):  ";
  for (int v : tree)
    std::cout << v << " ";
  std::cout << "\n";

  std::cout << "  in-order (reverse):  ";
  auto it = tree.end();
  --it;
  for (;;) {
    std::cout << *it << " ";
    if (it == tree.begin())
      break;
    --it;
  }
  std::cout << "\n";

  auto found = std::find(tree.begin(), tree.end(), 4);
  if (found != tree.end())
    std::cout << "  found 4 via std::find\n";
}

// ===========================================================================
// F. Sentinel Iterators (C++20)
// ===========================================================================

template <typename T> class ValueSentinel {
public:
  explicit ValueSentinel(T target) : target_(target) {}
  template <typename Iterator> bool operator==(const Iterator &it) const {
    return *it == target_;
  }

private:
  T target_;
};

void demo_sentinel() {
  section("F. Sentinel Iterators");

  std::vector<int> data = {10, 20, 30, 40, 50};
  ValueSentinel<int> stop(40);

  std::cout << "  elements before 40: ";
  for (auto it = data.begin(); it != stop; ++it) {
    std::cout << *it << " ";
  }
  std::cout << "\n";
}

// ===========================================================================
// G. Making Custom Type a Range
// ===========================================================================

class PrimeRange {
public:
  explicit PrimeRange(int max) : max_(max) {}

  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using pointer = const int *;
    using reference = const int &;

    iterator() : val_(0), max_(0) {}
    iterator(int start, int max) : val_(start), max_(max) {
      if (val_ < 2)
        val_ = 2;
      advance_to_prime();
    }

    reference operator*() const { return val_; }

    iterator &operator++() {
      ++val_;
      advance_to_prime();
      return *this;
    }
    iterator operator++(int) {
      auto tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const iterator &o) const { return val_ == o.val_; }
    bool operator!=(const iterator &o) const { return !(*this == o); }

  private:
    int val_, max_;

    static bool is_prime(int n) {
      if (n < 2)
        return false;
      if (n < 4)
        return true;
      if (n % 2 == 0 || n % 3 == 0)
        return false;
      for (int i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0)
          return false;
      return true;
    }

    void advance_to_prime() {
      while (val_ <= max_ && !is_prime(val_))
        ++val_;
      if (val_ > max_)
        val_ = max_;
    }
  };

  iterator begin() const { return iterator(2, max_); }
  iterator end() const { return iterator(max_ + 1, max_); }

private:
  int max_;
};

void demo_range_interop() {
  section("G. Making Custom Type a Range");

  PrimeRange primes(50);
  std::cout << "  primes up to 50: ";
  for (int p : primes)
    std::cout << p << " ";
  std::cout << "\n";

  auto count =
      std::count_if(primes.begin(), primes.end(), [](int p) { return p > 20; });
  std::cout << "  primes > 20: " << count << "\n";

  auto it = std::find(primes.begin(), primes.end(), 31);
  std::cout << "  found 31: " << (it != primes.end() ? "yes" : "no") << "\n";
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 2: Custom Iterators ===";

  demo_category_tags();
  demo_iterator_traits();
  demo_fibonacci_generator();
  demo_filtered_range();
  demo_tree_iterator();
  demo_sentinel();
  demo_range_interop();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
