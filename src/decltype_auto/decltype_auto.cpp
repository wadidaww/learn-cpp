/**
 * =============================================================================
 * LESSON: decltype and auto as Function Return Types in C++
 * =============================================================================
 *
 * This file teaches you about using `auto` and `decltype` for function return
 * types in C++11/14/17/20. We cover:
 *
 * 1. `auto` return type deduction (C++14)
 * 2. Trailing return type with `auto` + `-> decltype(...)` (C++11)
 * 3. `decltype(auto)` return type (C++14)
 * 4. Differences between `auto` and `decltype(auto)`
 * 5. Practical use cases and pitfalls
 *
 * Compile: g++ -std=c++17 -o decltype_auto decltype_auto.cpp
 * =============================================================================
 */

#include <iostream>
#include <string>
#include <vector>
#include <type_traits>

// =============================================================================
// SECTION 1: `auto` Return Type Deduction (C++14)
// =============================================================================
// The compiler deduces the return type from the return statement.
// Rules follow template argument deduction (references/cv-qualifiers are stripped).

auto add(int a, int b) {
    return a + b;  // deduced as int
}

auto make_greeting(const std::string& name) {
    return "Hello, " + name;  // deduced as std::string (value, not reference!)
}

// Multiple return statements must all deduce to the same type:
auto absolute(int x) {
    if (x >= 0) return x;   // int
    else return -x;          // int  — OK, same type
}

// =============================================================================
// SECTION 2: Trailing Return Type with decltype (C++11)
// =============================================================================
// Before C++14, we needed trailing return types to express complex return types.
// Syntax: auto func(params) -> decltype(expression)

// This was the C++11 way to write generic functions:
template <typename T, typename U>
auto multiply(T t, U u) -> decltype(t * u) {
    return t * u;
}

// Why trailing return type? Because `t` and `u` aren't in scope before the
// parameter list, so we can't write: decltype(t*u) multiply(T t, U u) {...}

// =============================================================================
// SECTION 3: `decltype(auto)` Return Type (C++14)
// =============================================================================
// `decltype(auto)` preserves the EXACT type, including references and
// cv-qualifiers. It applies decltype rules to the return expression.

// KEY DIFFERENCE:
//   auto        → strips references and cv-qualifiers (like template deduction)
//   decltype(auto) → preserves everything (applies decltype rules)

int global_value = 42;

// Returns int (value) — auto strips the reference
auto get_value_auto() {
    return global_value;  // deduced as int (copy!)
}

// Returns int& (reference) — decltype(auto) preserves the reference
decltype(auto) get_value_decltype() {
    return (global_value);  // decltype((global_value)) is int& 
    // NOTE: The parentheses are important! See Section 5.
}

// Returns int (value) — no parentheses, so decltype(global_value) is int
decltype(auto) get_value_no_parens() {
    return global_value;  // decltype(global_value) is int (named variable = its declared type)
}

// =============================================================================
// SECTION 4: Practical Comparison — auto vs decltype(auto)
// =============================================================================

std::vector<int> vec = {10, 20, 30};

// `auto` — always returns by value (a copy of the element)
auto get_element_auto(size_t idx) {
    return vec[idx];  // deduced as int (copy)
}

// `decltype(auto)` — returns exactly what the expression gives
// vec[idx] returns int&, so this returns int&
decltype(auto) get_element_decltype(size_t idx) {
    return vec[idx];  // deduced as int& (reference to the actual element!)
}

// =============================================================================
// SECTION 5: decltype Rules Refresher
// =============================================================================
// decltype(entity)     → declared type of the entity
// decltype(expression) → depends on value category:
//     - lvalue  → T&
//     - xvalue  → T&&
//     - prvalue → T
//
// IMPORTANT: decltype(x) vs decltype((x)):
//   int x;
//   decltype(x)   → int     (entity: declared type)
//   decltype((x)) → int&    (expression: x is an lvalue, so result is int&)

// =============================================================================
// SECTION 6: Generic Perfect-Forwarding Return with decltype(auto)
// =============================================================================
// One of the most powerful uses: wrapping functions while preserving return type.

int& get_ref() {
    static int val = 100;
    return val;
}

int get_val() {
    return 200;
}

// A generic wrapper that perfectly forwards the return type:
template <typename F, typename... Args>
decltype(auto) call_and_log(F&& f, Args&&... args) {
    std::cout << "  [LOG] Calling function...\n";
    // decltype(auto) preserves whether f returns by value or by reference
    return std::forward<F>(f)(std::forward<Args>(args)...);
}

// If we used `auto` here instead, references would be lost — the wrapper
// would always return by value, breaking code that relies on the reference.

// =============================================================================
// SECTION 7: auto with Trailing Return Type in Templates (Modern Style)
// =============================================================================
// In C++14+, you can often just use `auto`, but trailing return types are
// still useful for SFINAE or when you want to be explicit:

template <typename Container>
auto front(Container& c) -> decltype(c.front()) {
    return c.front();
}
// This won't compile if `c` doesn't have `.front()` — SFINAE-friendly!

// =============================================================================
// SECTION 8: Pitfalls and Gotchas
// =============================================================================

// PITFALL 1: Dangling reference with decltype(auto)
// NEVER return a local variable by reference!
/*
decltype(auto) dangling() {
    int local = 5;
    return (local);  // decltype((local)) = int& → DANGLING REFERENCE! UB!
}
*/

// PITFALL 2: auto in recursive functions
// The return type must be deducible before the recursive call:
/*
auto factorial(int n) {
    if (n <= 1) return 1;       // OK: deduced as int here
    return n * factorial(n-1);  // OK: already deduced from above
}
*/
// But if the recursive call comes first, it won't compile.

// PITFALL 3: auto strips const and reference
const std::string& get_name() {
    static const std::string name = "C++";
    return name;
}

auto stripped() {
    return get_name();  // returns std::string (copy! const& stripped)
}

decltype(auto) preserved() {
    return get_name();  // returns const std::string& (preserved!)
}

// =============================================================================
// MAIN — Demonstrations
// =============================================================================
int main() {
    std::cout << "=== SECTION 1: auto return type ===\n";
    std::cout << "  add(3, 4) = " << add(3, 4) << "\n";
    std::cout << "  make_greeting(\"World\") = " << make_greeting("World") << "\n";
    std::cout << "  absolute(-7) = " << absolute(-7) << "\n";

    std::cout << "\n=== SECTION 2: Trailing return type (C++11) ===\n";
    std::cout << "  multiply(3, 4.5) = " << multiply(3, 4.5) << "\n";
    std::cout << "  multiply(2.0f, 3) = " << multiply(2.0f, 3) << "\n";
    // multiply deduces: int*double=double, float*int=float

    std::cout << "\n=== SECTION 3: decltype(auto) vs auto ===\n";
    std::cout << "  global_value = " << global_value << "\n";

    auto val1 = get_value_auto();       // int (copy)
    val1 = 999;                          // doesn't affect global_value
    (void)val1;
    std::cout << "  After get_value_auto() modified: global_value = " << global_value << "\n";

    decltype(auto) ref1 = get_value_decltype();  // int& (reference)
    ref1 = 999;                                   // MODIFIES global_value!
    std::cout << "  After get_value_decltype() modified: global_value = " << global_value << "\n";

    std::cout << "\n=== SECTION 4: Vector element access ===\n";
    std::cout << "  vec = {" << vec[0] << ", " << vec[1] << ", " << vec[2] << "}\n";

    // get_element_auto(0) returns a copy — can't modify vec through it:
    // get_element_auto(0) = 100;  // ERROR: can't assign to rvalue

    get_element_decltype(1) = 77;  // Returns int&; MODIFIES vec[1]!
    std::cout << "  After get_element_decltype(1) = 77: vec = {"
              << vec[0] << ", " << vec[1] << ", " << vec[2] << "}\n";

    std::cout << "\n=== SECTION 6: Perfect forwarding return ===\n";
    // call_and_log preserves the reference return of get_ref()
    decltype(auto) ref2 = call_and_log(get_ref);
    std::cout << "  get_ref() via wrapper = " << ref2 << "\n";
    static_assert(std::is_lvalue_reference_v<decltype(ref2)>,
                  "ref2 should be int&");

    // call_and_log also works with value returns
    auto val2 = call_and_log(get_val);
    std::cout << "  get_val() via wrapper = " << val2 << "\n";

    std::cout << "\n=== SECTION 7: Trailing return for SFINAE ===\n";
    std::vector<std::string> names = {"Alice", "Bob", "Charlie"};
    std::cout << "  front(names) = " << front(names) << "\n";

    std::cout << "\n=== SECTION 8: Pitfall — auto strips qualifiers ===\n";
    auto s1 = stripped();    // std::string (copy)
    decltype(auto) s2 = preserved();  // const std::string&
    std::cout << "  stripped() is reference? "
              << std::is_reference_v<decltype(s1)> << "\n";       // 0 (false)
    std::cout << "  preserved() is reference? "
              << std::is_reference_v<decltype(s2)> << "\n";      // 1 (true)
    std::cout << "  preserved() is const? "
              << std::is_const_v<std::remove_reference_t<decltype(s2)>> << "\n"; // 1 (true)

    std::cout << "\n=== SUMMARY ===\n";
    std::cout << R"(
  +---------------------+-------------------------------------------+
  | Return Type         | Behavior                                  |
  +---------------------+-------------------------------------------+
  | auto                | Strips refs & cv-qualifiers (always copy) |
  | auto -> decltype(e) | Trailing return, C++11 compatible         |
  | decltype(auto)      | Preserves exact type (refs, const, etc.)  |
  +---------------------+-------------------------------------------+

  Use `auto` when you want simple value returns.
  Use `decltype(auto)` when you need to preserve references/const.
  Use trailing return types for SFINAE or C++11 compatibility.
)" << std::endl;

    return 0;
}
