// 08_type_erasure.cpp
//
// Lesson 8 — Type Erasure
//
// Type erasure lets you store different types behind a uniform interface
// without inheritance.  It combines value semantics with polymorphism.
//
// Classic examples:
//   - std::function<R(Args...)>  — erases any callable matching a signature
//   - std::any                   — erases any type
//   - std::pmr::memory_resource  — erases allocator strategy
//
// How it works (general pattern):
//   1. Define an abstract interface (pure virtual)
//   2. Derive a concrete class that holds the actual value
//   3. Store a pointer to the abstract interface
//   4. Forward calls through the interface
//
// Patterns shown:
//   A. std::function internals
//   B. Hand-rolled std::function (simplified)
//   C. Hand-rolled std::any (simplified)
//   D. std::pmr::memory_resource as type erasure (review)
//   E. Polymorphic wrapper: AnyCallable
//   F. Small buffer optimization (SBO) analysis

#include <any>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char *title) {
  std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. std::function Internals
// ===========================================================================

void demo_function_internals() {
  section("A. std::function Internals");

  std::cout << "  sizeof(std::function<void()>) = "
            << sizeof(std::function<void()>) << " bytes\n";
  std::cout << "  sizeof(void(*)())             = " << sizeof(void (*)())
            << " bytes\n";

  std::function<void()> f1 = []() { std::cout << "  lambda 1\n"; };
  std::cout << "  small lambda target type: " << f1.target_type().name()
            << "\n";

  int x = 42;
  std::function<void()> f2 = [x]() {
    std::cout << "  lambda 2: " << x << "\n";
  };
  std::cout << "  capturing lambda target type: " << f2.target_type().name()
            << "\n";

  f1();
  f2();
}

// ===========================================================================
// B. Hand-rolled std::function (Simplified)
// ===========================================================================

template <typename Signature> class MyFunction;

template <typename R, typename... Args> class MyFunction<R(Args...)> {
public:
  struct Concept {
    virtual ~Concept() = default;
    virtual R invoke(Args... args) = 0;
    virtual std::unique_ptr<Concept> clone() const = 0;
  };

  template <typename F> struct Model : Concept {
    F func_;
    explicit Model(F f) : func_(std::move(f)) {}
    R invoke(Args... args) override {
      return func_(std::forward<Args>(args)...);
    }
    std::unique_ptr<Concept> clone() const override {
      return std::make_unique<Model>(func_);
    }
  };

  MyFunction() = default;

  template <typename F, typename = std::enable_if_t<
                            !std::is_same_v<std::decay_t<F>, MyFunction>>>
  MyFunction(F f)
      : ptr_(std::make_unique<Model<std::decay_t<F>>>(std::move(f))) {}

  MyFunction(const MyFunction &other) {
    if (other.ptr_)
      ptr_ = other.ptr_->clone();
  }

  MyFunction(MyFunction &&) noexcept = default;
  MyFunction &operator=(MyFunction &&) noexcept = default;

  MyFunction &operator=(const MyFunction &other) {
    if (this != &other) {
      ptr_ = other.ptr_ ? other.ptr_->clone() : nullptr;
    }
    return *this;
  }

  R operator()(Args... args) {
    return ptr_->invoke(std::forward<Args>(args)...);
  }

  explicit operator bool() const { return ptr_ != nullptr; }

private:
  std::unique_ptr<Concept> ptr_;
};

void demo_handrolled_function() {
  section("B. Hand-rolled std::function");

  MyFunction<int(int, int)> add = [](int a, int b) { return a + b; };
  MyFunction<int(int, int)> mul = [](int a, int b) { return a * b; };

  std::cout << "  add(3, 4) = " << add(3, 4) << "\n";
  std::cout << "  mul(3, 4) = " << mul(3, 4) << "\n";

  add = [](int a, int b) { return a - b; };
  std::cout << "  after reassignment: add(10, 3) = " << add(10, 3) << "\n";

  MyFunction<void()> empty;
  std::cout << "  empty: " << static_cast<bool>(empty) << "\n";
  std::cout << "  add: " << static_cast<bool>(add) << "\n";
}

// ===========================================================================
// C. Hand-rolled std::any (Simplified)
// ===========================================================================

class MyAny {
public:
  struct Concept {
    virtual ~Concept() = default;
    virtual std::unique_ptr<Concept> clone() const = 0;
    virtual const std::type_info &type() const = 0;
  };

  template <typename T> struct Model : Concept {
    T value_;
    explicit Model(T v) : value_(std::move(v)) {}
    std::unique_ptr<Concept> clone() const override {
      return std::make_unique<Model>(value_);
    }
    const std::type_info &type() const override { return typeid(T); }
  };

  MyAny() = default;

  template <typename T>
  MyAny(T value) : ptr_(std::make_unique<Model<T>>(std::move(value))) {}

  MyAny(const MyAny &other) {
    if (other.ptr_)
      ptr_ = other.ptr_->clone();
  }

  MyAny(MyAny &&) noexcept = default;
  MyAny &operator=(const MyAny &other) {
    if (this != &other)
      ptr_ = other.ptr_ ? other.ptr_->clone() : nullptr;
    return *this;
  }
  MyAny &operator=(MyAny &&) noexcept = default;

  bool has_value() const { return ptr_ != nullptr; }

  template <typename T> friend T &any_cast(MyAny &any);

  template <typename T> friend const T &any_cast(const MyAny &any);

private:
  std::unique_ptr<Concept> ptr_;
};

template <typename T> T &any_cast(MyAny &any) {
  auto *model = dynamic_cast<MyAny::Model<T> *>(any.ptr_.get());
  if (!model)
    throw std::bad_cast();
  return model->value_;
}

template <typename T> const T &any_cast(const MyAny &any) {
  auto *model = dynamic_cast<const MyAny::Model<T> *>(any.ptr_.get());
  if (!model)
    throw std::bad_cast();
  return model->value_;
}

void demo_handrolled_any() {
  section("C. Hand-rolled std::any");

  MyAny a = 42;
  MyAny b = std::string("hello");
  MyAny c = 3.14;

  std::cout << "  a (int): " << any_cast<int>(a) << "\n";
  std::cout << "  b (string): " << any_cast<std::string>(b) << "\n";
  std::cout << "  c (double): " << any_cast<double>(c) << "\n";

  try {
    auto val = any_cast<std::string>(a);
    (void)val;
  } catch (const std::bad_cast &e) {
    std::cout << "  bad_cast caught: " << e.what() << "\n";
  }

  a = 100;
  std::cout << "  after reassignment: " << any_cast<int>(a) << "\n";

  MyAny empty;
  std::cout << "  empty has_value: " << empty.has_value() << "\n";
}

// ===========================================================================
// D. std::pmr::memory_resource as Type Erasure
// ===========================================================================

void demo_pmr_type_erasure() {
  section("D. pmr::memory_resource as Type Erasure (Review)");

  std::cout << "  The pattern:\n";
  std::cout
      << "    1. memory_resource is an abstract base (virtual interface)\n";
  std::cout << "    2. Custom resources derive and override "
               "do_allocate/do_deallocate\n";
  std::cout << "    3. polymorphic_allocator stores a memory_resource*\n";
  std::cout
      << "    4. pmr::vector<T> uses polymorphic_allocator<T> by default\n";
  std::cout << "    5. All pmr containers are the same type regardless of "
               "resource\n\n";

  std::cout << "  This is type erasure:\n";
  std::cout << "    - The container doesn't know the concrete resource type\n";
  std::cout << "    - Calls are dispatched through virtual functions\n";
  std::cout << "    - You can swap resources at runtime\n";
}

// ===========================================================================
// E. Polymorphic Wrapper: AnyCallable
// ===========================================================================

template <typename F> class AnyCallable;

template <typename R, typename... Args> class AnyCallable<R(Args...)> {
public:
  struct Concept {
    virtual ~Concept() = default;
    virtual R invoke(Args... args) = 0;
  };

  template <typename F> struct Model : Concept {
    F func_;
    explicit Model(F f) : func_(std::move(f)) {}
    R invoke(Args... args) override {
      return func_(std::forward<Args>(args)...);
    }
  };

  static constexpr std::size_t SBO_SIZE = 64;
  alignas(std::max_align_t) char buffer_[SBO_SIZE];
  Concept *ptr_ = nullptr;

public:
  AnyCallable() = default;

  template <typename F, typename = std::enable_if_t<
                            !std::is_same_v<std::decay_t<F>, AnyCallable>>>
  AnyCallable(F &&f) {
    using Decayed = std::decay_t<F>;
    using ModelType = Model<Decayed>;

    if (sizeof(ModelType) <= SBO_SIZE) {
      ptr_ = new (buffer_) ModelType(std::forward<F>(f));
    } else {
      ptr_ = new ModelType(std::forward<F>(f));
    }
  }

  ~AnyCallable() {
    if (ptr_) {
      if (reinterpret_cast<const char *>(ptr_) >= buffer_ &&
          reinterpret_cast<const char *>(ptr_) < buffer_ + SBO_SIZE) {
        ptr_->~Concept();
      } else {
        delete ptr_;
      }
    }
  }

  AnyCallable(const AnyCallable &) = delete;
  AnyCallable &operator=(const AnyCallable &) = delete;

  AnyCallable(AnyCallable &&o) noexcept {
    if (o.is_small()) {
      std::memcpy(buffer_, o.buffer_, SBO_SIZE);
      ptr_ = reinterpret_cast<Concept *>(buffer_);
    } else {
      ptr_ = o.ptr_;
    }
    o.ptr_ = nullptr;
  }

  AnyCallable &operator=(AnyCallable &&o) noexcept {
    if (this != &o) {
      this->~AnyCallable();
      new (this) AnyCallable(std::move(o));
    }
    return *this;
  }

  R operator()(Args... args) {
    return ptr_->invoke(std::forward<Args>(args)...);
  }

  explicit operator bool() const { return ptr_ != nullptr; }

private:
  bool is_small() const {
    return ptr_ && reinterpret_cast<const char *>(ptr_) >= buffer_ &&
           reinterpret_cast<const char *>(ptr_) < buffer_ + SBO_SIZE;
  }
};

// A simple thread pool that accepts any callable
class SimpleThreadPool {
public:
  using Task = AnyCallable<void()>;

  void enqueue(Task task) { tasks_.push_back(std::move(task)); }

  void run_all() {
    for (auto &task : tasks_)
      task();
    tasks_.clear();
  }

private:
  std::vector<Task> tasks_;
};

void demo_polymorphic_wrapper() {
  section("E. Polymorphic Wrapper: AnyCallable");

  SimpleThreadPool pool;

  pool.enqueue([]() { std::cout << "  task 1: lambda\n"; });

  int counter = 0;
  pool.enqueue([&counter]() {
    counter++;
    std::cout << "  task 2: capturing lambda, counter=" << counter << "\n";
  });

  struct Functor {
    void operator()() const { std::cout << "  task 3: function object\n"; }
  };
  pool.enqueue(Functor{});

  pool.run_all();
  std::cout << "  counter after tasks: " << counter << "\n";
}

// ===========================================================================
// F. Small Buffer Optimization (SBO) Analysis
// ===========================================================================

void demo_sbo_analysis() {
  section("F. Small Buffer Optimization (SBO)");

  std::cout << "  SBO stores small objects inline (no heap allocation).\n\n";

  auto measure = [](const char *name, auto callable) {
    using F = decltype(callable);
    std::cout << "  " << name << ":\n";
    std::cout << "    sizeof(callable) = " << sizeof(F) << " bytes\n";
    std::cout << "    model size = " << sizeof(AnyCallable<void()>::Model<F>)
              << " bytes\n";
    std::cout << "    SBO threshold = " << AnyCallable<void()>::SBO_SIZE
              << " bytes\n";
    std::cout << "    fits in SBO: "
              << (sizeof(AnyCallable<void()>::Model<F>) <=
                  AnyCallable<void()>::SBO_SIZE)
              << "\n";
  };

  measure("empty lambda", []() {});
  measure("lambda with int capture", [x = 42]() { (void)x; });
  measure("lambda with 8 ints",
          [a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, g = 7, h = 8]() {
            (void)a;
            (void)b;
            (void)c;
            (void)d;
            (void)e;
            (void)f;
            (void)g;
            (void)h;
          });

  std::cout << "\n  Benefits of SBO:\n";
  std::cout << "    - No heap allocation for small callables\n";
  std::cout << "    - Better cache locality\n";
  std::cout << "    - Faster construction/destruction\n";
  std::cout << "    - No shared_ptr overhead\n";
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 8: Type Erasure ===";

  demo_function_internals();
  demo_handrolled_function();
  demo_handrolled_any();
  demo_pmr_type_erasure();
  demo_polymorphic_wrapper();
  demo_sbo_analysis();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
