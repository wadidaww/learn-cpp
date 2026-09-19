// STL CONTAINERS
// ===============
// Key concepts:
//   Sequence:    vector (dynamic array), array (fixed), deque (front+back), list (doubly-linked), forward_list (singly-linked)
//   Associative: map (ordered tree), set (ordered), unordered_map/set (hash table)
//   Adaptor:     stack (LIFO), queue (FIFO), priority_queue (max-heap)
//   Utility:     tuple (heterogeneous), optional (nullable), variant (tagged union), span (non-owning view)
//
// When to use:
//   vector: default choice; O(1) append, O(1) random access, O(n) insert at front
//   deque: frequent push_front + push_back
//   list/forward_list: frequent insert/delete in middle (no pointer invalidation)
//   map: ordered key-value pairs (O(log n))
//   unordered_map: fast lookup (O(1) avg), no ordering
//   set: sorted unique elements
//   optional: function may or may not return a value
//   variant: type-safe union (visit with std::visit)

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <deque>
#include <list>
#include <forward_list>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <stack>
#include <queue>
#include <tuple>
#include <optional>
#include <variant>
#include <span>

int main() {
    // --- SEQUENCE CONTAINERS ---
    std::vector<int> v = {1, 2, 3, 4, 5};         // contiguous memory, resizable
    v.push_back(6); v.pop_back();
    std::cout << "vector: size=" << v.size() << " cap=" << v.capacity() << "\n";

    std::array<int, 3> arr = {10, 20, 30};         // fixed-size, stack-allocated
    std::deque<int> dq = {1, 2}; dq.push_front(0); // O(1) front+back insert
    std::list<std::string> lst = {"b", "a"}; lst.sort(); lst.push_front("c"); // O(1) insert/erase anywhere
    std::forward_list<int> fl = {3, 1}; fl.push_front(2); fl.reverse();        // singly-linked, less overhead

    // --- ASSOCIATIVE CONTAINERS ---
    std::map<std::string, int> ages = {{"Alice", 30}, {"Bob", 25}};        // ordered (red-black tree)
    ages["Charlie"] = 35;
    std::unordered_map<std::string, double> prices = {{"apple", 1.5}, {"banana", 0.75}}; // hash table
    std::set<int> s = {5, 3, 8, 1}; s.insert(4); s.erase(3);              // sorted unique
    std::unordered_set<std::string> tags = {"cpp", "modern"};              // hash set

    std::cout << "map: ";
    for (const auto& [k, v] : ages) std::cout << k << ":" << v << " ";
    std::cout << "\n";
    std::cout << "set: ";
    for (int i : s) std::cout << i << " "; std::cout << "\n";

    // --- ADAPTOR CONTAINERS ---
    std::stack<int> stk; stk.push(10); stk.push(20);             // LIFO
    std::queue<std::string> q; q.push("first"); q.push("second"); // FIFO
    std::priority_queue<int> pq; pq.push(5); pq.push(1); pq.push(9); // max-heap
    std::cout << "stack.top=" << stk.top() << " queue.front=" << q.front() << " heap.top=" << pq.top() << "\n";

    // --- UTILITY TYPES ---
    auto person = std::make_tuple("Alice", 30, 1.65);             // heterogeneous
    auto& [name, age, height] = person;                            // structured binding
    std::cout << "tuple: " << name << " age=" << age << "\n";

    auto find_value = [](int k) -> std::optional<int> { return k == 42 ? std::optional(42) : std::nullopt; };
    if (auto val = find_value(42)) std::cout << "optional: found " << *val << "\n";

    std::variant<int, double, std::string> v1 = "hello";          // type-safe union
    std::cout << "variant: " << std::get<std::string>(v1) << " (index=" << v1.index() << ")\n";

    std::vector<int> data = {10, 20, 30, 40, 50};
    std::span<int> view(data);                                    // non-owning view
    std::cout << "span: size=" << view.size() << " subspan(1,3)=" << view[1] << view[2] << view[3] << "\n";
}
