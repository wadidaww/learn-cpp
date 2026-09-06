# C++ Extension Points — from custom deleters to `std::formatter`

This directory teaches C++ extensibility points through ten progressive lessons.
Each lesson explores a different way the standard library lets you plug in your
own implementation.

---

## Table of Contents

1. [What are Extension Points?](#what-are-extension-points)
2. [Lessons Overview](#lessons-overview)
3. [Extension Points Reference](#extension-points-reference)
4. [Build Instructions](#build-instructions)
5. [Running the Examples](#running-the-examples)
6. [Key Concepts Reference](#key-concepts-reference)
7. [Best Practices](#best-practices)

---

## What are Extension Points?

The C++ standard library defines many **extension points** — interfaces where
you can plug in your own implementation to customize behavior.  Unlike simple
configuration, these extension points let you:

- **Customize container behavior** (allocators, comparators, hashers)
- **Extend type functionality** (streaming, formatting, iteration)
- **Inject behavior at compile time** (policies, concepts, traits)
- **Manage resources with type safety** (smart pointer deleters)
- **Add I/O sources/sinks** (custom streambuf)

---

## Lessons Overview

### Lesson 1 — `01_custom_deleters.cpp`

> **Custom deleters for smart pointers**

| Section | Topic |
|---------|-------|
| A | `FILE*` deleter — RAII wrapper for C file handles |
| B | Socket deleter — lambda deleter for network sockets |
| C | Lambda deleters — stateless and stateful |
| D | `shared_ptr` aliasing constructor |
| E | `unique_ptr` with array deleter |
| F | `unique_ptr` vs `shared_ptr` ownership semantics |

---

### Lesson 2 — `02_custom_iterators.cpp`

> **Custom iterators and iterator traits**

| Section | Topic |
|---------|-------|
| A | Iterator category tags (`input`, `forward`, `bidirectional`, `random_access`) |
| B | `std::iterator_traits` and `RangeGenerator` |
| C | Fibonacci generator (input iterator) |
| D | Filtered iterator adaptor |
| E | Tree iterator (bidirectional, in-order traversal) |
| F | Sentinel iterators (C++20 style) |
| G | Making custom type a range (`begin`/`end` interop) |

---

### Lesson 3 — `03_traits_specialization.cpp`

> **Type traits specialization**

| Section | Topic |
|---------|-------|
| A | `std::hash` specialization for `Point`, `Rectangle` |
| B | `std::char_traits` for case-insensitive character type |
| C | `std::equal_to` and `std::less` specialization |
| D | Custom type traits from scratch (`is_iterable`, `is_addable`, `is_hashable`) |
| E | SFINAE with `std::enable_if` |
| F | Detection idiom with `std::void_t` |

---

### Lesson 4 — `04_cpp20_concepts.cpp`

> **C++20 Concepts**

| Section | Topic |
|---------|-------|
| A | Defining concepts (`Sortable`, `Container`, `Hashable`) |
| B | Constraining function templates with `requires` |
| C | Constraining class templates |
| D | Requires expressions |
| E | Subsumption (concept refinement) |
| F | Concepts vs SFINAE comparison |
| G | Built-in concepts (`std::same_as`, `std::derived_from`, etc.) |

---

### Lesson 5 — `05_streambuf_customization.cpp`

> **Custom streambuf**

| Section | Topic |
|---------|-------|
| A | Streambuf anatomy (put/get area overview) |
| B | Circular buffer streambuf |
| C | Logging tee streambuf (console + file) |
| D | Memory-mapped file streambuf (read side) |
| E | Connecting custom streambuf to `istream`/`ostream` |
| F | Bidirectional streambuf (read + write) |

---

### Lesson 6 — `06_error_category.cpp`

> **Custom error category**

| Section | Topic |
|---------|-------|
| A | Defining error codes (`enum class`) |
| B | `std::error_category` subclass |
| C | `std::error_code` / `std::error_condition` |
| D | Error code to message mapping |
| E | `std::system_error` exception |
| F | Comparison with `boost::system::error_code` |

---

### Lesson 7 — `07_ranges_views.cpp`

> **Ranges and views (C++20)**

| Section | Topic |
|---------|-------|
| A | `std::ranges::range` concept |
| B | `std::ranges::view_interface` (CRTP base) |
| C | Custom range: `StepRange` |
| D | Pipe-able view adaptors (transform, filter) |
| E | Composing views with `std::ranges` |
| F | `std::ranges` algorithms |
| G | `std::views::iota`, `enumerate`, `zip` |

---

### Lesson 8 — `08_type_erasure.cpp`

> **Type erasure patterns**

| Section | Topic |
|---------|-------|
| A | `std::function` internals — virtual table + small buffer |
| B | Hand-rolled `std::function` (simplified) |
| C | Hand-rolled `std::any` (simplified) |
| D | `std::pmr::memory_resource` as type erasure (review) |
| E | Polymorphic wrapper: `AnyCallable<R(Args...)>` |
| F | Small buffer optimization (SBO) analysis |

---

### Lesson 9 — `09_policy_based_design.cpp`

> **Policy-based design**

| Section | Topic |
|---------|-------|
| A | Template policy parameters (growth strategies) |
| B | Static vs dynamic policy selection |
| C | CRTP Mixins (`Loggable`, `Timable`, `Validatable`) |
| D | Policy-based container (`FlexVector`) |
| E | Error handling policies (`Throw`, `Nothrow`, `Abort`) |
| F | Thread safety policies (`SingleThreaded`, `ThreadSafe`) |

---

### Lesson 10 — `10_custom_formatter.cpp`

> **Custom formatter (C++20)**

| Section | Topic |
|---------|-------|
| A | `std::formatter` concept and requirements |
| B | Basic formatter for `Point` |
| C | Custom format specifiers (`Color`) |
| D | Integration with `std::format` / `std::print` |
| E | Formatting containers of custom types |
| F | Compile-time format string validation |

---

## Extension Points Reference

| Extension Point | What You Specialize/Extend | Lesson |
|---|---|---|
| `std::unique_ptr<T, Deleter>` | Custom destruction for resources | 1 |
| `std::shared_ptr<T, Deleter>` | Custom deallocation + ref counting | 1 |
| `std::iterator_traits<It>` | Iterator metadata for algorithms | 2 |
| Custom iterator | `begin()`/`end()` + category tags | 2 |
| `std::hash<T>` | Use type in unordered containers | 3 |
| `std::char_traits<T>` | Custom character type for strings | 3 |
| Type traits (`is_iterable`, etc.) | Compile-time type queries | 3 |
| C++20 concepts | Constrain templates | 4 |
| `std::basic_streambuf` | Custom I/O source/sink | 5 |
| `std::error_category` | Custom error domain | 6 |
| `std::ranges::range` | Make type iterable | 7 |
| `std::ranges::view_interface` | Lazy view base class | 7 |
| Type erasure (`std::function`) | Value semantics + polymorphism | 8 |
| Policy classes | Compile-time behavior injection | 9 |
| `std::formatter<T>` | Custom type in `std::format` | 10 |

---

## Build Instructions

```bash
# From the repository root
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- \
    ext_01_deleters      \
    ext_02_iterators     \
    ext_03_traits        \
    ext_04_concepts      \
    ext_05_streambuf     \
    ext_06_errors        \
    ext_07_ranges        \
    ext_08_type_erasure  \
    ext_09_policies      \
    ext_10_formatter
```

Binaries are placed in `build/src/extension_points/`.

**Requirements:**
- C++20 compiler (GCC 13+, Clang 14+)
- C++20 `<format>` support (GCC 13+, Clang 14+)
- CMake 3.10+
- POSIX headers for mmap/socket examples

---

## Running the Examples

```bash
cd build/src/extension_points

./ext_01_deleters
./ext_02_iterators
./ext_03_traits
./ext_04_concepts
./ext_05_streambuf
./ext_06_errors
./ext_07_ranges
./ext_08_type_erasure
./ext_09_policies
./ext_10_formatter
```

---

## Key Concepts Reference

### Smart Pointer Deleters

```cpp
// Struct deleter
struct FileDeleter {
    void operator()(FILE* fp) const { std::fclose(fp); }
};
std::unique_ptr<FILE, FileDeleter> file(std::fopen("x.txt", "r"));

// Lambda deleter
auto deleter = [](int fd) { ::close(fd); };
std::unique_ptr<int, decltype(deleter)> sock(new int(fd), deleter);

// shared_ptr aliasing
auto sensor = std::make_shared<Sensor>(42);
std::shared_ptr<std::string> name(sensor, &sensor->name);
```

### Iterator Requirements

```cpp
struct MyIterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const T*;
    using reference         = const T&;

    reference operator*() const;
    MyIterator& operator++();
    MyIterator operator++(int);
    bool operator==(const MyIterator&) const;
};
```

### Concept Definition

```cpp
template <typename T>
concept Sortable = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
};

template <Sortable T>
void sort(std::vector<T>& v);
```

### Custom Formatter

```cpp
template <>
struct std::formatter<MyType> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.end();  // parse format spec
    }
    auto format(const MyType& val, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", val);
    }
};
```

### Type Erasure Pattern

```cpp
struct Concept {
    virtual ~Concept() = default;
    virtual void invoke() = 0;
};

template <typename F>
struct Model : Concept {
    F func_;
    void invoke() override { func_(); }
};
```

---

## Best Practices

| Practice | Reason |
|---|---|
| Use `std::unique_ptr` with custom deleters for RAII | Zero-overhead ownership of C resources |
| Specialize `std::hash` for custom map keys | Enables `unordered_map<YourType, V>` |
| Prefer concepts over SFINAE | Better error messages, self-documenting |
| Use `std::ranges` for lazy composition | No intermediate allocations |
| Use type erasure when you need value semantics + polymorphism | Avoids heap allocation for small objects |
| Prefer policy classes over virtual dispatch | Zero overhead, compile-time selection |
| Specialize `std::formatter` for `std::format` integration | Standard formatting ecosystem |
| Use `std::error_category` for domain-specific errors | Standard error handling framework |
