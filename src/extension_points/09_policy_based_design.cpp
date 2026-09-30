// 09_policy_based_design.cpp
//
// Lesson 9 — Policy-Based Design
//
// Policy-based design uses template parameters to inject behavior at
// compile time.  This is more flexible than inheritance because:
//
//   - No virtual dispatch overhead
//   - Policies can be composed freely
//   - Compile-time errors are caught early
//   - Can mix compile-time and runtime selection
//
// Key techniques:
//   - Policy classes as template parameters
//   - Template template parameters for policy families
//   - CRTP mixins for adding behavior
//   - Static vs dynamic policy selection
//
// Patterns shown:
//   A. Template policy parameters
//   B. Static vs dynamic policy selection
//   C. CRTP Mixins
//   D. Policy-based container (vector with growth strategy)
//   E. Error handling policies
//   F. Thread safety policies

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char *title) {
  std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. Template Policy Parameters
// ===========================================================================
// Policies are classes that define a specific aspect of behavior.
// They are passed as template parameters to customize the container.

struct DoubleGrowth {
  static std::size_t grow(std::size_t current) {
    return current == 0 ? 1 : current * 2;
  }
};

struct FixedIncrementGrowth {
  static std::size_t grow(std::size_t current) { return current + 1024; }
};

struct FibonacciGrowth {
  static std::size_t grow(std::size_t current) {
    if (current < 2)
      return current + 1;
    return static_cast<std::size_t>(current * 1.618);
  }
};

template <typename T, typename GrowthPolicy = DoubleGrowth> class PolicyVector {
public:
  PolicyVector() : data_(nullptr), size_(0), capacity_(0) {}

  explicit PolicyVector(std::size_t n) : size_(n), capacity_(n) {
    data_ = new T[n]{};
  }

  ~PolicyVector() { delete[] data_; }

  PolicyVector(const PolicyVector &) = delete;
  PolicyVector &operator=(const PolicyVector &) = delete;

  PolicyVector(PolicyVector &&o) noexcept
      : data_(o.data_), size_(o.size_), capacity_(o.capacity_) {
    o.data_ = nullptr;
    o.size_ = 0;
    o.capacity_ = 0;
  }

  void push_back(const T &val) {
    if (size_ == capacity_) {
      std::size_t new_cap = GrowthPolicy::grow(capacity_);
      if (new_cap <= capacity_)
        new_cap = capacity_ + 1;
      reserve(new_cap);
    }
    data_[size_++] = val;
  }

  void reserve(std::size_t new_cap) {
    if (new_cap <= capacity_)
      return;
    T *new_data = new T[new_cap]{};
    for (std::size_t i = 0; i < size_; ++i)
      new_data[i] = data_[i];
    delete[] data_;
    data_ = new_data;
    capacity_ = new_cap;
  }

  std::size_t size() const { return size_; }
  std::size_t capacity() const { return capacity_; }
  bool empty() const { return size_ == 0; }

  const T &operator[](std::size_t i) const { return data_[i]; }
  T &operator[](std::size_t i) { return data_[i]; }

private:
  T *data_;
  std::size_t size_;
  std::size_t capacity_;
};

void demo_policy_parameters() {
  section("A. Template Policy Parameters");

  PolicyVector<int, DoubleGrowth> dv;
  for (int i = 0; i < 10; ++i)
    dv.push_back(i * 10);
  std::cout << "  DoubleGrowth: size=" << dv.size()
            << " capacity=" << dv.capacity() << "\n";

  PolicyVector<int, FixedIncrementGrowth> fv;
  for (int i = 0; i < 10; ++i)
    fv.push_back(i * 10);
  std::cout << "  FixedIncrement: size=" << fv.size()
            << " capacity=" << fv.capacity() << "\n";

  PolicyVector<int, FibonacciGrowth> fig;
  for (int i = 0; i < 10; ++i)
    fig.push_back(i * 10);
  std::cout << "  Fibonacci: size=" << fig.size()
            << " capacity=" << fig.capacity() << "\n";
}

// ===========================================================================
// B. Static vs Dynamic Policy Selection
// ===========================================================================

// Static: compile-time selection (zero overhead)
template <typename Formatter> std::string format_static(int value) {
  return Formatter::format(value);
}

struct HexFormatter {
  static std::string format(int v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%X", v);
    return buf;
  }
};

struct DecFormatter {
  static std::string format(int v) { return std::to_string(v); }
};

struct BinFormatter {
  static std::string format(int v) {
    if (v == 0)
      return "0";
    std::string result;
    while (v > 0) {
      result = (v % 2 ? "1" : "0") + result;
      v /= 2;
    }
    return result;
  }
};

// Dynamic: runtime selection via function pointer or std::function
using DynamicFormatter = std::function<std::string(int)>;

std::string format_dynamic(int value, DynamicFormatter fmt) {
  return fmt(value);
}

void demo_static_vs_dynamic() {
  section("B. Static vs Dynamic Policy Selection");

  int val = 42;

  // Static: zero overhead, resolved at compile time
  std::cout << "  static hex: " << format_static<HexFormatter>(val) << "\n";
  std::cout << "  static dec: " << format_static<DecFormatter>(val) << "\n";
  std::cout << "  static bin: " << format_static<BinFormatter>(val) << "\n";

  // Dynamic: runtime flexibility, slight overhead
  DynamicFormatter hex_fmt = [](int v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%X", v);
    return std::string(buf);
  };
  std::cout << "  dynamic hex: " << format_dynamic(val, hex_fmt) << "\n";

  std::cout
      << "\n  Static: compile-time, zero overhead, can't change at runtime\n";
  std::cout << "  Dynamic: runtime flexibility, virtual dispatch overhead\n";
}

// ===========================================================================
// C. CRTP Mixins
// ===========================================================================
// Mixins add behavior to a class via inheritance from a template base.
// The base class receives the derived class via CRTP.

// Mixin: adds logging capability
template <typename Derived> class Loggable {
public:
  void log(const std::string &msg) const {
    std::cout << "  [" << static_cast<const Derived *>(this)->name() << "] "
              << msg << "\n";
  }
};

// Mixin: adds timing capability
template <typename Derived> class Timable {
public:
  void start_timer() { start_ = std::chrono::steady_clock::now(); }

  void stop_timer() {
    auto end = std::chrono::steady_clock::now();
    auto ms =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start_)
            .count();
    std::cout << "  [" << static_cast<const Derived *>(this)->name()
              << "] elapsed: " << ms << " us\n";
  }

private:
  std::chrono::steady_clock::time_point start_;
};

// Mixin: adds validation
template <typename Derived> class Validatable {
public:
  bool validate() const {
    return static_cast<const Derived *>(this)->do_validate();
  }
};

// Compose mixins into a concrete class
class DataProcessor : public Loggable<DataProcessor>,
                      public Timable<DataProcessor>,
                      public Validatable<DataProcessor> {
public:
  std::string name() const { return "DataProcessor"; }

  void process() {
    log("starting processing");
    start_timer();

    // Simulate work
    volatile int sum = 0;
    for (int i = 0; i < 1000000; ++i)
      sum += i;

    stop_timer();
    log("processing complete");
  }

  bool do_validate() const { return true; }
};

// Another class with different mixins
class NetworkService : public Loggable<NetworkService> {
public:
  std::string name() const { return "NetworkService"; }

  void send(const std::string &data) { log("sending: " + data); }
};

void demo_crtp_mixins() {
  section("C. CRTP Mixins");

  DataProcessor processor;
  processor.log("manual log message");
  processor.process();
  std::cout << "  valid: " << processor.validate() << "\n";

  NetworkService svc;
  svc.send("hello");
}

// ===========================================================================
// D. Policy-Based Container (Review with Policies)
// ===========================================================================
// A more complete vector with multiple policy slots.

struct ThrowOnOverflow {
  static void handle() { throw std::bad_alloc{}; }
};

struct AbortOnOverflow {
  static void handle() {
    std::cerr << "  [AbortOnOverflow] memory allocation failed\n";
    std::abort();
  }
};

template <typename T, typename GrowPolicy, typename OverflowPolicy>
class FlexVector {
public:
  FlexVector() : data_(nullptr), size_(0), cap_(0) {}
  ~FlexVector() { delete[] data_; }

  FlexVector(const FlexVector &) = delete;
  FlexVector &operator=(const FlexVector &) = delete;

  void push_back(const T &val) {
    if (size_ == cap_) {
      std::size_t new_cap = GrowPolicy::grow(cap_);
      if (new_cap <= cap_)
        OverflowPolicy::handle();
      reserve(new_cap);
    }
    data_[size_++] = val;
  }

  void reserve(std::size_t n) {
    if (n <= cap_)
      return;
    T *p = new T[n]{};
    for (std::size_t i = 0; i < size_; ++i)
      p[i] = data_[i];
    delete[] data_;
    data_ = p;
    cap_ = n;
  }

  std::size_t size() const { return size_; }
  std::size_t capacity() const { return cap_; }
  const T &operator[](std::size_t i) const { return data_[i]; }

private:
  T *data_;
  std::size_t size_;
  std::size_t cap_;
};

void demo_flex_vector() {
  section("D. Policy-Based Container");

  FlexVector<int, DoubleGrowth, ThrowOnOverflow> safe_vec;
  for (int i = 0; i < 5; ++i)
    safe_vec.push_back(i);
  std::cout << "  safe vector: size=" << safe_vec.size()
            << " cap=" << safe_vec.capacity() << "\n";

  FlexVector<int, FixedIncrementGrowth, ThrowOnOverflow> fixed_vec;
  for (int i = 0; i < 5; ++i)
    fixed_vec.push_back(i * 100);
  std::cout << "  fixed vector: size=" << fixed_vec.size()
            << " cap=" << fixed_vec.capacity() << "\n";
}

// ===========================================================================
// E. Error Handling Policies
// ===========================================================================

struct ThrowPolicy {
  static void handle_error(const char *msg) { throw std::runtime_error(msg); }
};

struct NothrowPolicy {
  static void handle_error(const char *msg) {
    std::cerr << "  [NothrowPolicy] error: " << msg << "\n";
  }
};

struct AbortPolicy {
  static void handle_error(const char *msg) {
    std::cerr << "  [AbortPolicy] fatal: " << msg << "\n";
    std::abort();
  }
};

template <typename T, typename ErrorPolicy = ThrowPolicy> class SafeContainer {
public:
  T &at(std::size_t i) {
    if (i >= size_) {
      ErrorPolicy::handle_error("index out of bounds");
    }
    return data_[i];
  }

  void add(T val) {
    if (size_ >= MAX) {
      ErrorPolicy::handle_error("container full");
      return;
    }
    data_[size_++] = val;
  }

  std::size_t size() const { return size_; }

private:
  static constexpr std::size_t MAX = 100;
  T data_[MAX]{};
  std::size_t size_ = 0;
};

void demo_error_policies() {
  section("E. Error Handling Policies");

  SafeContainer<int, ThrowPolicy> tc;
  tc.add(42);
  std::cout << "  throw container: at(0)=" << tc.at(0) << "\n";

  try {
    tc.at(5);
  } catch (const std::exception &e) {
    std::cout << "  caught: " << e.what() << "\n";
  }

  SafeContainer<int, NothrowPolicy> nc;
  nc.add(100);
  std::cout << "  nothrow container: at(0)=" << nc.at(0) << "\n";
  nc.at(5);
}

// ===========================================================================
// F. Thread Safety Policies
// ===========================================================================

struct SingleThreaded {
  struct Lock {
    Lock(const SingleThreaded &) {}
  };
};

struct ThreadSafe {
  mutable std::mutex mtx_;
  struct Lock {
    std::lock_guard<std::mutex> guard;
    Lock(const ThreadSafe &ts) : guard(ts.mtx_) {}
  };
};

template <typename T, typename ThreadPolicy = SingleThreaded>
class ThreadSafeContainer : private ThreadPolicy {
  using Lock = typename ThreadPolicy::Lock;

public:
  void push(T val) {
    Lock lock(*this);
    data_.push_back(std::move(val));
  }

  T pop() {
    Lock lock(*this);
    T val = std::move(data_.back());
    data_.pop_back();
    return val;
  }

  std::size_t size() const {
    Lock lock(*this);
    return data_.size();
  }

private:
  std::vector<T> data_;
};

void demo_thread_safety() {
  section("F. Thread Safety Policies");

  ThreadSafeContainer<int, SingleThreaded> stc;
  stc.push(1);
  stc.push(2);
  stc.push(3);
  std::cout << "  single-threaded container size: " << stc.size() << "\n";

  ThreadSafeContainer<int, ThreadSafe> tsc;
  tsc.push(10);
  tsc.push(20);
  std::cout << "  thread-safe container size: " << tsc.size() << "\n";

  // Demonstrate concurrent access
  ThreadSafeContainer<int, ThreadSafe> shared;
  auto producer = [&shared]() {
    for (int i = 0; i < 100; ++i)
      shared.push(i);
  };

  {
    std::thread t1(producer);
    std::thread t2(producer);
    t1.join();
    t2.join();
  }
  std::cout << "  concurrent push (2 threads x 100): size=" << shared.size()
            << "\n";
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 9: Policy-Based Design ===";

  demo_policy_parameters();
  demo_static_vs_dynamic();
  demo_crtp_mixins();
  demo_flex_vector();
  demo_error_policies();
  demo_thread_safety();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
