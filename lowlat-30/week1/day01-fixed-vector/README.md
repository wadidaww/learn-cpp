# Day 1 · Value categories, move semantics, object lifetime

**Why trading firms care:** hot-path code never touches the heap, so you build your own fixed-capacity containers. Doing that correctly forces you to understand exactly when objects are created, moved and destroyed. Interviewers use "write a small vector" to test this in 30 minutes.

## 1. Concepts (45 min)

### Value categories
Every expression has a type *and* a category.

| Category | Has identity? | Can be moved from? | Example |
|---|---|---|---|
| lvalue | yes | no (unless you cast) | `x`, `*p`, `v[0]` |
| xvalue | yes | yes | `std::move(x)`, `f()` returning `T&&` |
| prvalue | no | yes (it materialises) | `42`, `T{}`, `f()` returning `T` |

- glvalue = lvalue + xvalue. rvalue = xvalue + prvalue.
- `std::move(x)` is only `static_cast<T&&>(x)`. It moves nothing. The move constructor that then runs does the work.
- Since C++17, a prvalue used to initialise an object of the same type is not a temporary at all: `T t = T{};` constructs once (guaranteed copy elision).

### Reference binding
- `T&` binds lvalues. `const T&` binds anything. `T&&` binds rvalues only.
- Inside a function, a named `T&&` parameter is an **lvalue** (it has a name). That is why you must `std::move` it again to pass it on.

### Forwarding references and perfect forwarding
```cpp
template <class U> void push(U&& u) {         // U&& with U deduced = forwarding reference
    store(std::forward<U>(u));                 // lvalue stays lvalue, rvalue stays rvalue
}
```
Reference collapsing: `& &`→`&`, `& &&`→`&`, `&& &`→`&`, `&& &&`→`&&`. So `U = T&` gives `T&`, and `U = T` gives `T&&`.

### Rule of 0 / 3 / 5
- **Rule of 0:** if members manage themselves (`std::vector`, `std::unique_ptr`), write no special members.
- **Rule of 5:** if you write any of destructor / copy ctor / copy assign / move ctor / move assign, think about all five. Declaring a destructor or copy op suppresses the implicit move ops, so moves silently become copies.
- Mark move ops `noexcept`. `std::vector` only moves elements on reallocation if the move constructor is `noexcept` (strong exception guarantee); otherwise it copies.

### Object lifetime with raw storage
A fixed-capacity vector owns bytes, not objects. You start and end lifetimes by hand:
```cpp
alignas(T) std::byte buf_[N * sizeof(T)];
T* slot(std::size_t i) { return std::launder(reinterpret_cast<T*>(buf_) + i); }
std::construct_at(slot(i), args...);   // begins lifetime (placement new)
std::destroy_at(slot(i));              // ends lifetime (calls ~T)
```
- `std::byte buf[N]` without `alignas(T)` is a bug: misaligned `T` is UB and slow.
- Do **not** use `T data_[N]`: it default-constructs N objects (and fails to compile for non-default-constructible `T`).
- `std::launder` tells the compiler a new object now lives at that address. Using the pointer returned by `construct_at` is always fine; laundering matters when you re-derive the pointer from the buffer.

### Exception safety words to know
- **No-throw:** never fails (`noexcept` moves, destructors).
- **Strong:** if it throws, state is unchanged (commit-or-rollback).
- **Basic:** if it throws, no leaks and invariants hold, but state may change.

## 2. Exercise (2 h): `ll::FixedVector<T, N>`

Write the bodies in `solution.hpp`. Every member is declared already; the tests will not link until you define them.

Requirements:
- No heap allocation. Storage is an aligned byte array inside the object.
- Works for types with **no default constructor** and **move-only** types.
- `push_back` / `emplace_back` return `false` / `nullptr` when full (no exceptions on the hot path).
- Copy and move constructors/assignment do the right thing element by element. A moved-from vector is valid (choose: empty).
- Destructor destroys exactly the live elements.
- Self-assignment is safe.
- Move ops are `noexcept` when `T`'s are (`noexcept(std::is_nothrow_move_constructible_v<T>)`).

Build and run:
```bash
cmake -S lowlat-30 -B build && cmake --build build --target day01_test && ./build/day01_test
cmake -S lowlat-30 -B build-asan -DLL_SANITIZE=address,undefined && cmake --build build-asan --target day01_test && ./build-asan/day01_test
```

**Done when:** all tests pass under ASan+UBSan, and you can explain each line without looking.

Stretch: add `insert(pos, value)` and `erase(pos)` with correct lifetimes. Then put `FixedVector<int, 8>` on godbolt and check `push_back` compiles to a compare plus a store.

## 3. Say it out loud (30 min)

<details><summary>1. What does std::move do?</summary>
It casts its argument to an rvalue reference (an xvalue). Nothing moves until a move constructor or move assignment runs.
</details>

<details><summary>2. Why should a move constructor be noexcept?</summary>
Containers like std::vector use std::move_if_noexcept when reallocating. If the move can throw, they copy to keep the strong guarantee, which is slower and impossible for move-only types.
</details>

<details><summary>3. You declared a destructor. What happened to the move constructor?</summary>
A user-declared destructor (or copy op) suppresses the implicit move constructor and move assignment. Moves fall back to copies. Declare them `= default` if you want them.
</details>

<details><summary>4. Why not `T data_[N]` in a fixed vector?</summary>
It constructs N objects up front, needs a default constructor, and runs N destructors even for empty slots. Raw aligned storage plus construct_at/destroy_at constructs only live elements.
</details>

<details><summary>5. Inside `void f(Widget&& w)`, is `w` an lvalue or rvalue?</summary>
An lvalue: it has a name. Its type is rvalue-reference-to-Widget, but the expression `w` is an lvalue. Use std::move(w) to pass it on as an rvalue.
</details>

<details><summary>6. What is guaranteed copy elision?</summary>
Since C++17, initialising an object from a prvalue of the same type constructs it directly in place. `T t = make_T();` (when make_T returns a prvalue) never calls a copy or move constructor; it does not even need one to exist.
</details>

<details><summary>7. When is NRVO not applied?</summary>
NRVO (returning a named local) is allowed but not guaranteed. It usually fails when different return statements return different named objects, or you return a function parameter. Then the return is treated as an rvalue and moved.
</details>

<details><summary>8. What breaks if storage is not alignas(T)?</summary>
Constructing T at a misaligned address is undefined behaviour. On x86 it often "works" but can be slower or crash with aligned SIMD loads; on other CPUs it can fault.
</details>
