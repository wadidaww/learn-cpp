// SMART POINTERS
// ==============
// Key concepts:
//   1. unique_ptr: single ownership, zero-overhead, move-only
//   2. shared_ptr: reference-counted shared ownership, last owner destroys object
//   3. weak_ptr: non-owning observer, breaks circular references
//   4. make_shared/make_unique: prefer over new (exception-safe, single allocation)

#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Widget {
    std::string name;
    explicit Widget(std::string n) : name(std::move(n)) { std::cout << "  Widget(\"" << name << "\") created\n"; }
    ~Widget() { std::cout << "  Widget(\"" << name << "\") destroyed\n"; }
    void greet() const { std::cout << "  Hello from Widget(\"" << name << "\")\n"; }
};

void demo_unique_ptr() {
    std::cout << "=== unique_ptr (single ownership) ===\n";
    auto w1 = std::make_unique<Widget>("alpha");
    w1->greet();
    std::unique_ptr<Widget> w2 = std::move(w1); // transfer ownership
    std::cout << "  w1: " << (w1 ? "valid" : "null") << ", w2 valid\n";
    // unique_ptr with arrays
    auto arr = std::make_unique<int[]>(5);
    for (int i = 0; i < 5; ++i) arr[i] = i * 10;
    // custom deleter
    { std::unique_ptr<Widget, decltype([](Widget* p){ delete p; })> w3(new Widget("beta")); }
}

void demo_shared_ptr() {
    std::cout << "\n=== shared_ptr (reference-counted) ===\n";
    auto sp1 = std::make_shared<Widget>("shared1");
    std::cout << "  use_count: " << sp1.use_count() << "\n";
    { auto sp2 = sp1; std::cout << "  use_count after copy: " << sp1.use_count() << "\n"; }
    std::cout << "  use_count after scope: " << sp1.use_count() << "\n";
}

void demo_weak_ptr() {
    std::cout << "\n=== weak_ptr (break cycles) ===\n";
    struct Node {
        std::string name;
        std::shared_ptr<Node> next;  // strong reference forward
        std::weak_ptr<Node> prev;    // weak reference backward (NO cycle)
        explicit Node(std::string n) : name(std::move(n)) {}
    };
    auto n1 = std::make_shared<Node>("A");
    auto n2 = std::make_shared<Node>("B");
    n1->next = n2; n2->prev = n1;  // prev is weak, so no leak
    if (auto locked = n2->prev.lock()) std::cout << "  n2->prev: " << locked->name << "\n";
}

int main() {
    demo_unique_ptr();
    demo_shared_ptr();
    demo_weak_ptr();
}
