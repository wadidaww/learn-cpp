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

template<typename K, typename V>
void print_map(const std::string& label, const std::map<K,V>& m) {
    std::cout << "  " << label << ": {";
    for (const auto& [k,v] : m) std::cout << k << ":" << v << " ";
    std::cout << "}\n";
}

int main() {
    // --- 1. vector (dynamic array) ---
    std::cout << "=== 1. vector ===\n";
    std::vector<int> v = {1, 2, 3, 4, 5};
    v.push_back(6);
    v.emplace_back(7);
    v.pop_back();
    std::cout << "  size=" << v.size() << " cap=" << v.capacity() << "\n";
    for (int i : v) std::cout << i << " ";
    std::cout << "\n";
    std::cout << "  v[2]=" << v.at(2) << " v.front()=" << v.front() << " v.back()=" << v.back() << "\n";

    // --- 2. array (fixed-size) ---
    std::cout << "\n=== 2. array ===\n";
    std::array<int, 5> arr = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < arr.size(); ++i) std::cout << arr[i] << " ";
    std::cout << "\n";

    // --- 3. deque (double-ended queue) ---
    std::cout << "\n=== 3. deque ===\n";
    std::deque<int> dq;
    dq.push_back(1);
    dq.push_back(2);
    dq.push_front(0);
    dq.push_front(-1);
    for (int i : dq) std::cout << i << " ";
    std::cout << "\n";

    // --- 4. list (doubly linked list) ---
    std::cout << "\n=== 4. list ===\n";
    std::list<std::string> lst = {"banana", "apple", "cherry"};
    lst.sort();
    lst.push_front("date");
    for (const auto& s : lst) std::cout << s << " ";
    std::cout << "\n";

    // --- 5. forward_list (singly linked list) ---
    std::cout << "\n=== 5. forward_list ===\n";
    std::forward_list<int> fl = {3, 1, 4};
    fl.push_front(2);
    fl.reverse();
    for (int i : fl) std::cout << i << " ";
    std::cout << "\n";

    // --- 6. map (ordered, red-black tree) ---
    std::cout << "\n=== 6. map ===\n";
    std::map<std::string, int> ages;
    ages["Alice"] = 30;
    ages["Bob"] = 25;
    ages["Charlie"] = 35;
    ages.insert({"Diana", 28});
    print_map("ages", ages);
    std::cout << "  Bob's age: " << ages["Bob"] << "\n";
    std::cout << "  count(Eve): " << ages.count("Eve") << "\n";

    // --- 7. unordered_map (hash table) ---
    std::cout << "\n=== 7. unordered_map ===\n";
    std::unordered_map<std::string, double> prices = {
        {"apple", 1.50}, {"banana", 0.75}, {"cherry", 3.00}
    };
    for (const auto& [fruit, price] : prices)
        std::cout << "  " << fruit << ": $" << price << "\n";

    // --- 8. set ---
    std::cout << "\n=== 8. set ===\n";
    std::set<int> s = {5, 3, 8, 1, 9, 2, 7};
    std::cout << "  sorted: ";
    for (int i : s) std::cout << i << " ";
    std::cout << "\n";
    s.insert(4);
    s.erase(9);
    std::cout << "  after insert(4) erase(9): ";
    for (int i : s) std::cout << i << " ";
    std::cout << "\n";

    // --- 9. unordered_set ---
    std::cout << "\n=== 9. unordered_set ===\n";
    std::unordered_set<std::string> tags = {"cpp", "modern", "stl", "cpp20"};
    std::cout << "  has 'cpp': " << tags.count("cpp") << "\n";
    std::cout << "  has 'rust': " << tags.count("rust") << "\n";

    // --- 10. stack (LIFO) ---
    std::cout << "\n=== 10. stack ===\n";
    std::stack<int> stk;
    stk.push(10);
    stk.push(20);
    stk.push(30);
    std::cout << "  top: " << stk.top() << "\n";
    stk.pop();
    std::cout << "  after pop, top: " << stk.top() << "\n";

    // --- 11. queue (FIFO) and priority_queue ---
    std::cout << "\n=== 11. queue & priority_queue ===\n";
    std::queue<std::string> q;
    q.push("first");
    q.push("second");
    q.push("third");
    std::cout << "  front: " << q.front() << " back: " << q.back() << "\n";

    std::priority_queue<int> pq;
    pq.push(5);
    pq.push(1);
    pq.push(9);
    pq.push(3);
    std::cout << "  priority_queue (max-heap): ";
    while (!pq.empty()) { std::cout << pq.top() << " "; pq.pop(); }
    std::cout << "\n";

    // --- 12. tuple ---
    std::cout << "\n=== 12. tuple ===\n";
    auto person = std::make_tuple("Alice", 30, 1.65);
    auto& [name, age, height] = person;
    std::cout << "  " << name << ", age " << age << ", height " << height << "\n";
    std::cout << "  get<0>: " << std::get<0>(person) << "\n";

    // --- 13. optional ---
    std::cout << "\n=== 13. optional ===\n";
    auto find_value = [](int key) -> std::optional<int> {
        if (key == 42) return 42;
        return std::nullopt;
    };
    if (auto val = find_value(42))
        std::cout << "  found: " << *val << "\n";
    if (auto val = find_value(99))
        std::cout << "  found: " << *val << "\n";
    else
        std::cout << "  99 not found\n";

    // --- 14. variant ---
    std::cout << "\n=== 14. variant ===\n";
    std::variant<int, double, std::string> v1 = "hello";
    std::cout << "  type index: " << v1.index() << "\n";
    std::cout << "  value: " << std::get<std::string>(v1) << "\n";
    v1 = 42;
    std::cout << "  after assign int: " << std::get<int>(v1) << "\n";

    // --- 15. span (non-owning view, C++20) ---
    std::cout << "\n=== 15. span ===\n";
    std::vector<int> data = {10, 20, 30, 40, 50};
    std::span<int> view(data);
    std::cout << "  span size: " << view.size() << "\n";
    std::cout << "  first: " << view.front() << " last: " << view.back() << "\n";
    std::cout << "  subspan(1,3): ";
    for (int i : view.subspan(1, 3)) std::cout << i << " ";
    std::cout << "\n";

    std::cout << "\nDone.\n";
    return 0;
}
