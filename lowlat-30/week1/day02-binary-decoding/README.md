# Day 2 · Undefined behaviour, aliasing, alignment, binary decoding

**Why trading firms care:** exchange feeds (Nasdaq ITCH, CME MDP3, Eurex EOBI) are packed binary messages. A feed handler turns bytes into structs millions of times per second. The fast, wrong way (`reinterpret_cast` a packet to a struct) is undefined behaviour, and interviewers ask about it. Today's message format is the real **ITCH 5.0 Add Order**, so day 20 reuses this code.

## 1. Concepts (45 min)

### The UB list you must be able to recite
| UB | Example | Why the optimiser cares |
|---|---|---|
| Signed overflow | `int x = INT_MAX; x + 1` | Compiler assumes `i + 1 > i`, removes checks, vectorises loops |
| Out-of-bounds access | `a[n]` | Anything |
| Use after free / lifetime end | dangling reference | Anything |
| Uninitialised read | `int x; return x;` | Value may differ between reads |
| Data race | two threads, one writes, no sync | Compiler keeps values in registers |
| Strict aliasing violation | read `float` through `int*` | Compiler assumes the two pointers do not overlap |
| Misaligned access | `*(uint32_t*)(buf + 1)` | UB in C++; x86 tolerates it, ARM may fault, SIMD may fault |
| Null dereference, shift ≥ width | `1u << 32` | Anything |

Unsigned overflow wraps and is **defined**. That is why sequence numbers and ring indices are unsigned.

### Strict aliasing
You may access an object only through its own type, a signed/unsigned variant, or `char` / `unsigned char` / `std::byte`. So:
```cpp
// UB: reads bytes as a uint32_t object that never existed, and may be misaligned.
uint32_t bad(const std::byte* p) { return *reinterpret_cast<const uint32_t*>(p); }

// Correct and just as fast: compilers turn a fixed-size memcpy into one mov.
uint32_t good(const std::byte* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
```
`std::bit_cast<To>(from)` (C++20) reinterprets one object's bytes as another type of the same size, by value. Use it for `double ↔ uint64_t`, not for buffers (it needs an object, not a pointer).

### Endianness
x86 and ARM (as normally run) are little-endian. Network protocols and ITCH are **big-endian**. C++23 has `std::byteswap`; in C++20 use `__builtin_bswap32/64` or write it with shifts (compilers recognise the pattern and emit `bswap` or `movbe`):
```cpp
uint32_t load_be32(const std::byte* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
```
Check on godbolt that this is one `movbe` or `mov`+`bswap` at `-O2`.

### Packed structs
`#pragma pack(1)` / `__attribute__((packed))` structs match the wire layout but make misaligned members, and taking `&member` gives a pointer you must not dereference as `T*`. Firms do use them; know the trade-off: layout matches wire, but every field read may be a misaligned load, and endianness still needs swapping.

### Fixed-point prices
ITCH prices are `uint32` with 4 implied decimals: `1234500` means `123.4500`. Never convert to `double` on the hot path: `0.1 + 0.2 != 0.3`, and two equal prices must compare equal for book lookup.

## 2. Exercise (2 h): decode and encode ITCH 5.0 Add Order

Wire layout (36 bytes, big-endian):

| Offset | Len | Field | Type |
|---|---|---|---|
| 0 | 1 | Message type | `'A'` |
| 1 | 2 | Stock locate | u16 |
| 3 | 2 | Tracking number | u16 |
| 5 | 6 | Timestamp (ns since midnight) | u48 |
| 11 | 8 | Order reference number | u64 |
| 19 | 1 | Buy/sell indicator | `'B'` or `'S'` |
| 20 | 4 | Shares | u32 |
| 24 | 8 | Stock | 8 ASCII chars, space padded |
| 32 | 4 | Price | u32, 4 implied decimals |

Write in `solution.hpp`:
- `load_be<T>` / `store_be<T>` for `uint16_t`, `uint32_t`, `uint64_t` (no `reinterpret_cast` of the buffer).
- `load_be48` / `store_be48` for the 6-byte timestamp.
- `decode_add_order(span, out)` returning a `DecodeError`.
- `encode_add_order(msg, span)` returning bytes written, or 0 if the span is too small.

**Done when:** tests pass under ASan+UBSan (the misaligned-buffer test catches the `reinterpret_cast` shortcut), and on godbolt your `load_be<uint64_t>` is a single `movbe` or `mov`+`bswap`.

Stretch: write the same decoder with a `#pragma pack(1)` struct and compare the generated assembly.

## 3. Say it out loud

<details><summary>1. Why is `*(uint32_t*)(buf+1)` wrong even on x86?</summary>
Two reasons: strict aliasing (no uint32_t object lives there) and misalignment. Both are UB. x86 tolerates the misaligned load in hardware, but the compiler may still optimise on the assumption that it never happens, and the same code faults on some ARM targets or with aligned SIMD loads. Use memcpy; it compiles to the same single load.
</details>

<details><summary>2. Is memcpy slow for 4 bytes?</summary>
No. A memcpy with a constant small size is a compiler builtin and becomes one load instruction.
</details>

<details><summary>3. Why does signed overflow being UB make code faster?</summary>
The compiler can assume `i + 1 > i`, so loop counters need no wrap checks, trip counts are known, and loops can be vectorised or strength-reduced. With wrapping (`-fwrapv`) some of that is lost.
</details>

<details><summary>4. Which types may alias anything?</summary>
char, unsigned char and std::byte. Reading any object's bytes through them is allowed.
</details>

<details><summary>5. Why fixed-point instead of double for prices?</summary>
Exactness: equal prices compare equal and can index a price level array; no rounding drift in sums; integer math is cheap and deterministic. Doubles are fine for signals and statistics, not for book keys or money.
</details>

<details><summary>6. What does std::bit_cast require?</summary>
Source and destination have the same size and are both trivially copyable. It returns a new object by value, and it is constexpr.
</details>

<details><summary>7. How do you detect UB in tests?</summary>
-fsanitize=address,undefined in CI, -fsanitize=thread for races, plus -Wall -Wextra -Wconversion. Fuzz parsers with libFuzzer so malformed input hits the edge cases.
</details>
