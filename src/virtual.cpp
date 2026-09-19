// VIRTUAL FUNCTIONS & POLYMORPHISM
// =================================
// Key concepts:
//   1. virtual: runtime dispatch via vtable (dynamic polymorphism)
//   2. override: explicitly mark derived function that overrides base virtual
//   3. Pure virtual (= 0): makes class abstract, cannot be instantiated
//   4. CRTP (Curiously Recurring Template Pattern): static polymorphism, no vtable overhead
//   5. struct padding: data members are aligned for performance (sizeof depends on order)

#include <iostream>
#include <memory>
#include <vector>

// --- 1. Classic virtual functions: runtime polymorphism ---
class Animal {
public:
    virtual ~Animal() = default;
    virtual void speak() const = 0;  // pure virtual → abstract class
    virtual std::string type() const { return "Animal"; }
};

class Dog : public Animal {
public:
    void speak() const override { std::cout << "  Woof!\n"; }
    std::string type() const override { return "Dog"; }
};

class Cat : public Animal {
public:
    void speak() const override { std::cout << "  Meow!\n"; }
    std::string type() const override { return "Cat"; }
};

// --- 2. CRTP: compile-time polymorphism (no vtable) ---
template<typename Derived>
class Base {
public:
    void interface() { static_cast<Derived*>(this)->implementation(); }
    void common() { std::cout << "  Base common\n"; }
};

class ImplA : public Base<ImplA> {
public:
    void implementation() { std::cout << "  ImplA::implementation()\n"; }
};

class ImplB : public Base<ImplB> {
public:
    void implementation() { std::cout << "  ImplB::implementation()\n"; }
};

// --- 3. Struct padding: member order affects sizeof ---
struct WithPadding  { char a; double c; int b; };  // 1+7pad+8+4+4pad = 24 bytes
struct WithoutPadding { char a; int b; double c; }; // 1+3pad+4+8 = 16 bytes

int main() {
    // Virtual dispatch
    std::cout << "=== Virtual Functions ===\n";
    std::vector<std::unique_ptr<Animal>> animals;
    animals.push_back(std::make_unique<Dog>());
    animals.push_back(std::make_unique<Cat>());
    for (const auto& a : animals) {
        std::cout << "  " << a->type() << ": ";
        a->speak();  // calls derived version at runtime
    }

    // CRTP: static dispatch, no vtable
    std::cout << "\n=== CRTP (Static Polymorphism) ===\n";
    ImplA a; a.interface();
    ImplB b; b.interface();

    // Struct padding
    std::cout << "\n=== Struct Padding ===\n";
    std::cout << "  WithPadding: " << sizeof(WithPadding) << " bytes\n";
    std::cout << "  WithoutPadding: " << sizeof(WithoutPadding) << " bytes\n";
    std::cout << "  Tip: order members largest-first to minimize padding\n";
}
