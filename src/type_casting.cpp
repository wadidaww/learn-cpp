// TYPE CASTING
// =============
// Key concepts:
//   static_cast:      compile-time, handles numeric/enum/upcast/downcast (unchecked)
//   dynamic_cast:     runtime, safe downcast via RTTI (returns nullptr/bad_cast on failure)
//   const_cast:       adds/removes const only — UB if you modify originally-const object
//   reinterpret_cast: raw memory reinterpretation — almost always wrong
//   C-style cast:     dangerous, can silently invoke reinterpret_cast — avoid in C++

#include <iostream>
#include <string>
#include <typeinfo>
#include <cstdint>

void demo_static_cast() {
    std::cout << "=== static_cast (compile-time, unchecked) ===\n";
    double d = 3.14;
    int i = static_cast<int>(d);  // numeric conversion
    std::cout << "  double→int: " << d << " → " << i << "\n";

    // upcast (derived→base) is always safe; downcast is unchecked
    struct Base { virtual void foo() {} };
    struct Derived : Base { void foo() override { std::cout << "  Derived::foo()\n"; } };
    Derived d_obj;
    Base& b_ref = static_cast<Base&>(d_obj);     // upcast: safe
    Derived& d_ref = static_cast<Derived&>(b_ref); // downcast: unchecked, works here
    d_ref.foo();
}

void demo_dynamic_cast() {
    std::cout << "\n=== dynamic_cast (runtime, safe downcast via RTTI) ===\n";
    struct Animal { virtual ~Animal() = default; virtual void speak() = 0; };
    struct Dog : Animal { void speak() override { std::cout << "  Woof!\n"; } };
    struct Cat : Animal { void speak() override { std::cout << "  Meow!\n"; } };

    Dog dog; Animal* ap = &dog;
    if (Dog* dp = dynamic_cast<Dog*>(ap)) dp->speak();           // success
    if (!dynamic_cast<Cat*>(ap)) std::cout << "  dynamic_cast<Cat*> failed (nullptr)\n";
    try { dynamic_cast<Cat&>(*ap); } catch (const std::bad_cast& e) { std::cout << "  " << e.what() << "\n"; }
}

void demo_const_cast() {
    std::cout << "\n=== const_cast (add/remove const only) ===\n";
    int x = 42;
    const int* cp = &x;
    int* p = const_cast<int*>(cp);  // safe: original x is non-const
    *p = 100;
    std::cout << "  modified via const_cast: " << x << "\n";
    std::cout << "  WARNING: modifying a truly const object is UNDEFINED BEHAVIOR\n";
}

void demo_reinterpret_cast() {
    std::cout << "\n=== reinterpret_cast (raw memory reinterpretation) ===\n";
    int x = 0x41424344;
    char* cp = reinterpret_cast<char*>(&x);
    std::cout << "  int→bytes: "; for (size_t i = 0; i < sizeof(int); ++i) std::cout << cp[i]; std::cout << "\n";

    uintptr_t addr = reinterpret_cast<uintptr_t>(&x);
    int* p = reinterpret_cast<int*>(addr);
    std::cout << "  ptr→int→ptr: " << *p << "\n";
}

int main() {
    demo_static_cast();
    demo_dynamic_cast();
    demo_const_cast();
    demo_reinterpret_cast();
    std::cout << "\n=== Summary ===\n";
    std::cout << "  static_cast:     safe numeric/polymorphic casts (default choice)\n";
    std::cout << "  dynamic_cast:    safe downcast with RTTI (needs virtual dtor)\n";
    std::cout << "  const_cast:      strip/add const only\n";
    std::cout << "  reinterpret_cast: raw memory reinterpretation (dangerous)\n";
}
