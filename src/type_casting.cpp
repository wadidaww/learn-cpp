#include <iostream>
#include <string>
#include <memory>
#include <typeinfo>

// --- 1. static_cast ---
void demo_static_cast() {
    std::cout << "=== 1. static_cast ===\n";

    // Numeric conversions
    double d = 3.14159;
    int i = static_cast<int>(d);
    std::cout << "  double→int: " << d << " → " << i << "\n";

    // Enum conversion
    enum Color { RED, GREEN, BLUE };
    int color_val = 1;
    Color c = static_cast<Color>(color_val);
    std::cout << "  int→enum: " << color_val << " → " << c << "\n";

    // Explicit upcast (derived→base)
    struct Base { virtual void foo() {} };
    struct Derived : Base { void foo() override { std::cout << "  Derived::foo()\n"; } };

    Derived d_obj;
    Base& b_ref = static_cast<Base&>(d_obj);  // upcast, safe
    // Calling through base won't dispatch to Derived::foo without virtual

    // Explicit downcast (base→derived) — use only when you're sure
    Derived& d_ref = static_cast<Derived&>(b_ref);  // downcast, unchecked
    d_ref.foo();

    // void* → typed pointer
    int val = 42;
    void* vptr = &val;
    int* iptr = static_cast<int*>(vptr);
    std::cout << "  void*→int*: " << *iptr << "\n";
}

// --- 2. dynamic_cast ---
void demo_dynamic_cast() {
    std::cout << "\n=== 2. dynamic_cast ===\n";

    struct Animal { virtual ~Animal() = default; virtual void speak() = 0; };
    struct Dog : Animal { void speak() override { std::cout << "  Woof!\n"; } };
    struct Cat : Animal { void speak() override { std::cout << "  Meow!\n"; } };

    Dog dog;
    Animal* ap = &dog;

    // Successful downcast
    if (Dog* dp = dynamic_cast<Dog*>(ap)) {
        std::cout << "  dynamic_cast<Dog*> succeeded: ";
        dp->speak();
    }

    // Failed downcast → nullptr
    if (Cat* cp = dynamic_cast<Cat*>(ap)) {
        std::cout << "  dynamic_cast<Cat*> succeeded\n";
    } else {
        std::cout << "  dynamic_cast<Cat*> failed (returned nullptr)\n";
    }

    // Reference cast — throws std::bad_cast on failure
    try {
        Cat& cr = dynamic_cast<Cat&>(*ap);
        (void)cr;
    } catch (const std::bad_cast& e) {
        std::cout << "  dynamic_cast<Cat&> threw: " << e.what() << "\n";
    }
}

// --- 3. const_cast ---
void demo_const_cast() {
    std::cout << "\n=== 3. const_cast ===\n";

    int x = 42;
    const int* cp = &x;

    // Remove const (if original is non-const, this is safe)
    int* p = const_cast<int*>(cp);
    *p = 100;
    std::cout << "  modified through const_cast: " << x << "\n";

    // Add const
    const int& cr = x;
    const int* cp2 = &cr;
    std::cout << "  const ref value: " << cr << "\n";

    // ⚠️ Undefined behavior: modifying through cast if original was const
    // const int y = 10;
    // int* bad = const_cast<int*>(&y);
    // *bad = 20; // UNDEFINED BEHAVIOR
    std::cout << "  WARNING: const_cast is only safe when removing const\n";
    std::cout << "  from a non-const object. Modifying a const object is UB.\n";
}

// --- 4. reinterpret_cast ---
void demo_reinterpret_cast() {
    std::cout << "\n=== 4. reinterpret_cast ===\n";

    int x = 0x41424344;  // "ABCD" in ASCII
    char* cp = reinterpret_cast<char*>(&x);
    std::cout << "  int→char*: ";
    for (size_t i = 0; i < sizeof(int); ++i)
        std::cout << cp[i];
    std::cout << "\n";

    // Pointer to integer
    int y = 42;
    uintptr_t addr = reinterpret_cast<uintptr_t>(&y);
    std::cout << "  pointer→integer: 0x" << std::hex << addr << std::dec << "\n";

    // Integer back to pointer
    int* p = reinterpret_cast<int*>(addr);
    std::cout << "  integer→pointer: " << *p << "\n";

    // Function pointer reinterpret (use case: serialization)
    using Func = int(*)(int, int);
    using OtherFunc = void(*)(void);
    Func f = [](int a, int b) { return a + b; };
    // reinterpret_cast between unrelated function types is UB, shown for syntax only
    std::cout << "  reinterpret_cast between unrelated function pointer types is UB\n";

    // Type punning via reinterpret_cast
    float fval = 3.14f;
    uint32_t bits = *reinterpret_cast<uint32_t*>(&fval);
    std::cout << "  float bits: 0x" << std::hex << bits << std::dec << "\n";
}

// --- 5. C-style casts (avoid) ---
void demo_c_style_cast() {
    std::cout << "\n=== 5. C-Style Casts (for comparison) ===\n";

    double d = 3.14;

    // C-style cast — tries static_cast, then const_cast, then reinterpret_cast
    int i = (int)d;                    // same as static_cast<int>(d)
    int j = int(d);                    // functional cast, same as above
    std::cout << "  C-style cast: " << d << " → " << i << "\n";
    std::cout << "  functional cast: " << d << " → " << j << "\n";

    std::cout << "  C-style casts are dangerous because they can silently\n";
    std::cout << "  invoke reinterpret_cast. Prefer C++ named casts.\n";
}

// --- 6. Common pitfalls ---
void demo_pitfalls() {
    std::cout << "\n=== 6. Common Pitfalls ===\n";

    struct Base { int a = 1; virtual ~Base() = default; };
    struct Middle : Base { int b = 2; };
    struct Derived : Middle { int c = 3; };

    Derived d;
    Base* bp = &d;

    // ❌ Dangerous: C-style downcast
    // Middle* mp = (Middle*)bp;  // unchecked, could be wrong

    // ✅ Safe: dynamic_cast
    if (auto mp = dynamic_cast<Middle*>(bp)) {
        std::cout << "  safe downcast: b = " << mp->b << "\n";
    }

    // ✅ Safe: static_cast when you're certain of the type
    Middle* mp2 = static_cast<Middle*>(bp);  // OK here because bp points to Derived
    std::cout << "  static_cast (known type): b = " << mp2->b << "\n";
}

int main() {
    demo_static_cast();
    demo_dynamic_cast();
    demo_const_cast();
    demo_reinterpret_cast();
    demo_c_style_cast();
    demo_pitfalls();
    std::cout << "\nDone.\n";
    return 0;
}
