# Variadic arguments and variadic templates

This mini-lesson is split into two source files:

- `c_style_variadics.cpp` — the old C-style `...` interface based on `va_list`
- `template_variadics.cpp` — the C++ type-safe approach based on template parameter packs

## Why there are two syntaxes

### 1. C-style variadics: `...`, `va_list`, `va_start`, `va_arg`, `va_copy`, `va_end`

Use this when you must interoperate with C APIs or old-style interfaces such as `printf`.

Mental model:

- the compiler only knows about the named parameters
- the unnamed arguments are read manually at runtime
- you are responsible for knowing how many arguments exist and what their types are

Core rules:

- the last named parameter is passed to `va_start`
- every `va_start` must have a matching `va_end`
- if you need to read the same arguments twice, make a second cursor with `va_copy`
- default promotions apply:
  - `bool`, `char`, and `short` are read as `int`
  - `float` is read as `double`
- reading the wrong type with `va_arg` is undefined behavior

Use C-style variadics for:

- compatibility with C libraries
- wrappers around C APIs
- legacy interfaces you cannot redesign

Avoid them for:

- new C++ APIs
- compile-time computation
- generic code that should be type-safe

### 2. Variadic templates: `template<typename... Ts>`, `template<auto... Values>`

Use this when you want a modern C++ interface.

Mental model:

- the compiler sees every argument in the pack
- the pack can hold types, values, or function arguments
- you can compute over the pack at compile time or expand it into generated code

Core rules:

- `typename... Ts` captures a pack of types
- `Ts... args` uses that pack as function parameters
- `sizeof...(Ts)` or `sizeof...(args)` gives pack length
- folds such as `(args + ... + 0)` reduce a pack without manual recursion
- `std::index_sequence` helps when you need pack positions

Use template variadics for:

- type-safe replacements for `printf`-style utilities
- forwarding wrappers
- tuple utilities
- compile-time type inspection
- metaprogramming and compile-time computation

## How to study these files

1. Read `c_style_variadics.cpp` first.
2. Compile and run it.
3. Notice how every example needs an external contract like a count or a layout string.
4. Read `template_variadics.cpp` second.
5. Compare how the compiler now knows the pack length and element types.
6. Focus on the `static_assert` examples: those are the compile-time computation patterns.

## Suggested commands

```bash
g++ -std=c++20 src/variadic/c_style_variadics.cpp -o /tmp/c_style_variadics
/tmp/c_style_variadics

g++ -std=c++20 src/variadic/template_variadics.cpp -o /tmp/template_variadics
/tmp/template_variadics
```

## What to practice next

- write a `min_value(args...)` with a fold expression
- write a `make_vector(args...)` that deduces a common type
- write `count_type_v<T, Ts...>` without looking at the solution
- write `nth_type_t<I, Ts...>` again from memory
- build a tiny `printf` replacement with variadic templates instead of `va_list`

If your goal is compile-time computation, spend most of your time on `template_variadics.cpp`. `va_list` is important to know, but it is mainly for runtime interoperability and legacy APIs.
