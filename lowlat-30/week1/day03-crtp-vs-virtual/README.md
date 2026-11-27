# Day 3 · Templates, CRTP vs virtual, concepts, `if constexpr`

**Why trading firms care:** the hot path is "on every market data update, run the strategy". Calling through a virtual function there costs an indirect branch and, more importantly, blocks inlining. Firms use CRTP, templates and concepts to choose the strategy at compile time. Expect: "what does a virtual call cost, and how do you avoid it?"

## 1. Concepts (45 min)

### How a virtual call works
- Each polymorphic class has one **vtable** (array of function pointers) in read-only memory.
- Each object carries a hidden **vptr** (8 bytes, usually at offset 0) pointing to its class's vtable.
- `p->f()` compiles to: load vptr from `*p`, load function pointer from `vptr[slot]`, indirect `call`.

The cost, in the order interviewers want to hear it:
1. **No inlining.** The compiler cannot see which function runs, so it cannot inline it, constant-fold through it, or vectorise the loop around it. This is usually the biggest cost.
2. **Indirect branch prediction.** Predictable when the same target repeats (monomorphic call site). Mispredicts (~15–20 cycles each) when targets alternate unpredictably.
3. **Two dependent loads** (vptr, then slot). Cheap if cached, a cache miss if the object or vtable is cold.
4. **8 bytes per object** for the vptr, which can push a struct over a cache line.

**Devirtualisation:** the compiler removes the indirect call when it can prove the dynamic type: the class or function is `final`, the object's type is known locally, or LTO/whole-program analysis sees one implementation. Know that `final` exists for this reason.

### CRTP (Curiously Recurring Template Pattern)
```cpp
template <class Derived>
struct StrategyBase {
    std::int64_t on_tick(const Tick& t) {            // no virtual
        return static_cast<Derived*>(this)->on_tick_impl(t);
    }
};
struct Momentum : StrategyBase<Momentum> {
    std::int64_t on_tick_impl(const Tick& t);
};
```
The base knows the derived type at compile time, so the call is direct and inlinable. Price: no runtime polymorphism (you cannot put `Momentum` and `Imbalance` in one `std::vector<Base*>`), more template instantiations (code size, compile time).

C++23 "deducing this" (`void on_tick(this auto& self, ...)`) does the same without the template parameter. Mention it; firms on C++20 still write CRTP.

### Other ways to avoid virtual
- **Concepts + templates:** `template <Strategy S> void run(S& s)`. The cleanest modern form.
- **`std::variant` + `std::visit`:** closed set of types, value semantics, no heap. `visit` is usually a jump table (similar to a virtual call) but objects are stored inline, and with few alternatives compilers often generate a compare chain that inlines.
- **Configure once, run monomorphic:** pick the type at startup and instantiate the whole engine for it (`Engine<Momentum>`). Most HFT systems do this.

### Concepts
```cpp
template <class S>
concept Strategy = requires(S s, const Tick& t) {
    { s.on_tick(t) } -> std::same_as<std::int64_t>;
};
```
Better error messages than SFINAE, and they document the interface.

### `if constexpr` and `[[likely]]`
- `if constexpr (cond)` discards the false branch at compile time. Use it for per-type paths inside a template.
- `[[likely]]` / `[[unlikely]]` change block layout (hot path falls through, cold path moves away). They do not change the branch predictor. Use them on error paths.

## 2. Exercise (2 h)

In `solution.hpp`, implement two strategies, each in **three** forms (virtual, CRTP, variant):

- **MidMomentum:** let `m = bid + ask` (twice the mid, no division). Return `m - previous_m`; return 0 on the first tick.
- **Imbalance:** return `(bid_qty - ask_qty) * 1000 / (bid_qty + ask_qty)` using integer division; return 0 if both quantities are 0.

All prices and quantities are `int64_t` fixed point. Then implement the free functions `run_virtual`, `run_crtp`, `run_variant` that return the sum of signals over a tick sequence.

**Done when:**
- tests pass;
- you run `bench` and can explain each number (`cmake --build build --target day03_bench && ./build/day03_bench`);
- on godbolt you have seen the `call [rax+16]`-style indirect call in the virtual loop and the inlined arithmetic in the CRTP loop;
- you tried marking `VMidMomentum` as `final` and calling through a `VMidMomentum&` and saw the call disappear.

## 3. Say it out loud

<details><summary>1. What does a virtual call cost?</summary>
Two dependent loads (vptr, slot) and an indirect call, usually well predicted when the target repeats. The real cost is losing inlining and every optimisation that follows from it. When targets vary unpredictably, add ~15–20 cycles per mispredict.
</details>

<details><summary>2. Where does the vptr live and when is it set?</summary>
Inside each object, typically at offset 0. Each constructor in the chain sets it to its own class's vtable, which is why calling a virtual function from a base constructor calls the base version.
</details>

<details><summary>3. CRTP downsides?</summary>
No runtime polymorphism or heterogeneous containers, code bloat per instantiation, longer compiles, harder error messages (concepts help), and the base must never be used on its own.
</details>

<details><summary>4. variant/visit vs virtual?</summary>
Variant: closed set of types, objects stored by value (no heap, better locality), exhaustive handling enforced at compile time. Virtual: open set, objects usually on the heap behind pointers. Dispatch cost is similar; locality is often better with variant.
</details>

<details><summary>5. What enables devirtualisation?</summary>
`final` on the class or function, the compiler knowing the exact type (local object, just constructed), or LTO / -fwhole-program seeing one override. Speculative devirtualisation (compare vptr, then inline the likely target) also exists in GCC/Clang.
</details>

<details><summary>6. Do [[likely]]/[[unlikely]] train the branch predictor?</summary>
No. They change code layout so the hot path is straight-line and the cold path is moved out of the way, which helps the instruction cache and front end.
</details>

<details><summary>7. How would you design pluggable strategies with zero virtual calls?</summary>
Make the engine a template on the strategy (`Engine<S>` with a `Strategy` concept), choose S from config at startup with a single switch that instantiates the right engine, then run monomorphic forever.
</details>
