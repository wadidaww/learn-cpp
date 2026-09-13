#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>

void print(const std::string& label, int value) {
    std::cout << "  " << label << " = " << value << "\n";
}

int main() {
    // --- 1. Basic lambda ---
    std::cout << "=== 1. Basic Lambda ===\n";
    auto greet = []() { std::cout << "  Hello from lambda!\n"; };
    greet();

    // --- 2. Parameters and return type ---
    std::cout << "\n=== 2. Parameters & Return ===\n";
    auto add = [](int a, int b) -> int { return a + b; };
    print("add(3,4)", add(3, 4));

    // --- 3. Captures ---
    std::cout << "\n=== 3. Captures ===\n";
    int x = 10, y = 20;

    auto by_value = [x]() { print("captured x", x); };
    auto by_ref = [&y]() { y = 99; };
    auto all_value = [=]() { return x + y; };   // capture all by value
    auto all_ref = [&]() { x = 50; y = 60; };  // capture all by reference

    by_value();
    by_ref();
    print("y after ref capture", y);
    print("all_value", all_value());
    all_ref();
    print("x after all_ref", x);
    print("y after all_ref", y);

    // --- 4. Mutable lambda ---
    std::cout << "\n=== 4. Mutable Lambda ===\n";
    int counter = 0;
    auto inc = [counter]() mutable -> int { return ++counter; };
    print("inc()", inc());
    print("inc()", inc());
    print("inc()", inc());
    print("counter (unchanged)", counter);  // original not modified

    // --- 5. Generic / templated lambda (C++14) ---
    std::cout << "\n=== 5. Generic Lambda ===\n";
    auto multiply = [](auto a, auto b) { return a * b; };
    print("multiply(3,4)", multiply(3, 4));
    std::cout << "  multiply(2.5, 3.0) = " << multiply(2.5, 3.0) << "\n";

    // --- 6. std::function ---
    std::cout << "\n=== 6. std::function ===\n";
    std::function<int(int, int)> op;
    op = [](int a, int b) { return a + b; };
    print("op(2,3)", op(2, 3));
    op = [](int a, int b) { return a * b; };
    print("op(2,3)", op(2, 3));

    // --- 7. Lambdas with STL algorithms ---
    std::cout << "\n=== 7. STL Algorithms ===\n";
    std::vector<int> nums = {5, 3, 8, 1, 9, 2, 7};

    std::sort(nums.begin(), nums.end(), [](int a, int b) { return a > b; });
    std::cout << "  sorted desc: ";
    for (int n : nums) std::cout << n << " ";
    std::cout << "\n";

    int sum = std::accumulate(nums.begin(), nums.end(), 0);
    print("sum", sum);

    int count = std::count_if(nums.begin(), nums.end(), [](int n) { return n > 5; });
    print("count > 5", count);

    auto it = std::find_if(nums.begin(), nums.end(), [](int n) { return n == 8; });
    if (it != nums.end()) print("found 8 at index", static_cast<int>(it - nums.begin()));

    // --- 8. IIFE (Immediately Invoked Function Expression) ---
    std::cout << "\n=== 8. IIFE ===\n";
    int result = [](int a, int b) { return a * b; }(3, 7);
    print("IIFE(3,7)", result);

    // --- 9. Recursive lambda via std::function ---
    std::cout << "\n=== 9. Recursive Lambda (factorial) ===\n";
    std::function<int(int)> factorial = [&](int n) -> int {
        return n <= 1 ? 1 : n * factorial(n - 1);
    };
    print("factorial(5)", factorial(5));

    // --- 10. Lambda as callback / closure ---
    std::cout << "\n=== 10. Lambda as Callback ===\n";
    auto on_complete = [](const std::string& msg) {
        std::cout << "  callback: " << msg << "\n";
    };

    auto do_work = [&](int iterations, std::function<void(const std::string&)> cb) {
        for (int i = 0; i < iterations; ++i) {
            std::cout << "  working... step " << i << "\n";
        }
        cb("done!");
    };
    do_work(3, on_complete);

    // --- 11. Generic lambda with perfect forwarding ---
    std::cout << "\n=== 11. Forwarding Lambda ===\n";
    auto forwarder = [](auto&&... args) {
        std::cout << "  forwarded args received\n";
    };
    int a = 1;
    std::string s = "hello";
    forwarder(a, s);

    std::cout << "\nDone.\n";
    return 0;
}
