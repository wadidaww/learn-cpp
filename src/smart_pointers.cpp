#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct Widget {
    std::string name;
    explicit Widget(std::string n) : name(std::move(n)) {
        std::cout << "  Widget(\"" << name << "\") constructed\n";
    }
    ~Widget() {
        std::cout << "  Widget(\"" << name << "\") destroyed\n";
    }
    void greet() const {
        std::cout << "  Hello from Widget(\"" << name << "\")\n";
    }
};

// --- 1. unique_ptr ---
void demo_unique_ptr() {
    std::cout << "=== unique_ptr ===\n";

    auto w1 = std::make_unique<Widget>("alpha");
    w1->greet();

    // Transfer ownership
    std::unique_ptr<Widget> w2 = std::move(w1);
    std::cout << "  w1 after move: " << (w1 ? "valid" : "null") << "\n";
    w2->greet();

    // unique_ptr with arrays
    auto arr = std::make_unique<int[]>(5);
    for (int i = 0; i < 5; ++i) arr[i] = i * 10;
    std::cout << "  array: ";
    for (int i = 0; i < 5; ++i) std::cout << arr[i] << " ";
    std::cout << "\n";

    // Custom deleter via lambda
    auto custom_deleter = [](Widget* p) {
        std::cout << "  [custom deleter] deleting " << p->name << "\n";
        delete p;
    };
    {
        std::unique_ptr<Widget, decltype(custom_deleter)> w3(
            new Widget("beta"), custom_deleter);
        w3->greet();
    } // custom_deleter called here

    // releasing ownership (raw pointer)
    Widget* raw = w2.release();
    std::cout << "  w2 after release: " << (w2 ? "valid" : "null") << "\n";
    raw->greet();
    delete raw; // manual cleanup required
}

// --- 2. shared_ptr ---
void demo_shared_ptr() {
    std::cout << "\n=== shared_ptr ===\n";

    std::shared_ptr<Widget> sp1 = std::make_shared<Widget>("shared1");
    std::cout << "  use_count after creation: " << sp1.use_count() << "\n";
    {
        std::shared_ptr<Widget> sp2 = sp1;  // shared ownership
        std::cout << "  use_count after copy: " << sp1.use_count() << "\n";
        sp2->greet();
    } // sp2 destroyed, use_count drops to 1
    std::cout << "  use_count after scope: " << sp1.use_count() << "\n";

    // shared_ptr with vector
    std::vector<std::shared_ptr<Widget>> widgets;
    widgets.push_back(std::make_shared<Widget>("vec1"));
    widgets.push_back(std::make_shared<Widget>("vec2"));
    std::cout << "  vector size: " << widgets.size() << "\n";
    std::cout << "  widget1 use_count: " << widgets[0].use_count() << "\n";
}

// --- 3. weak_ptr ---
void demo_weak_ptr() {
    std::cout << "\n=== weak_ptr (break cycles) ===\n";

    struct Node {
        std::string name;
        std::shared_ptr<Node> next;   // strong reference forward
        std::weak_ptr<Node> prev;     // weak reference backward (no cycle)

        explicit Node(std::string n) : name(std::move(n)) {
            std::cout << "  Node(\"" << name << "\") created\n";
        }
        ~Node() {
            std::cout << "  Node(\"" << name << "\") destroyed\n";
        }
    };

    auto n1 = std::make_shared<Node>("A");
    auto n2 = std::make_shared<Node>("B");

    n1->next = n2;       // A → B (strong)
    n2->prev = n1;       // B → A (weak, no cycle!)

    std::cout << "  n1 use_count: " << n1.use_count() << "\n";
    std::cout << "  n2 use_count: " << n2.use_count() << "\n";

    // Access via weak_ptr
    if (auto locked = n2->prev.lock()) {
        std::cout << "  n2's prev: " << locked->name << "\n";
    }
}

// --- 4. make_unique vs new ---
void demo_factory() {
    std::cout << "\n=== Factory / make_unique ===\n";

    auto make_widget = [](const std::string& name) -> std::unique_ptr<Widget> {
        return std::make_unique<Widget>(name);
    };

    auto w = make_widget("factory_widget");
    w->greet();

    // shared_ptr with aliasing constructor
    auto sp = std::make_shared<Widget>("owner");
    std::shared_ptr<std::string> name_alias(sp, &sp->name);
    std::cout << "  aliasing: " << *name_alias << "\n";
    std::cout << "  owner use_count: " << sp.use_count() << "\n";
}

int main() {
    demo_unique_ptr();
    demo_shared_ptr();
    demo_weak_ptr();
    demo_factory();
    std::cout << "\nDone.\n";
    return 0;
}
