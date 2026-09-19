// LAMBDA EXPRESSIONS
// ===================
// Key concepts:
//   1. Syntax: [captures](params) -> ret { body }
//   2. Captures: [=] by value, [&] by ref, [x] specific, [x = expr] init-capture (C++14)
//   3. Mutable lambdas: allow modifying captured-by-value copies
//   4. Generic lambdas (C++14): auto parameters create templates
//   5. Lambdas + algorithms: pass custom predicates to sort, find_if, transform, etc.

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>

int main() {
    // 1. Basic: lambda is an anonymous function object
    auto greet = []() { std::cout << "  Hello from lambda!\n"; };
    greet();

    // 2. Captures: [x] copies x, [&x] references x, [=]/[&] capture all
    int x = 10, y = 20;
    auto by_val = [x]() { std::cout << "  captured x=" << x << "\n"; };
    auto by_ref = [&y]() { y = 99; };
    by_val(); by_ref();
    std::cout << "  y after ref capture: " << y << "\n";

    // 3. Mutable: copy-captured variables are const by default; mutable removes that
    int counter = 0;
    auto inc = [counter]() mutable -> int { return ++counter; };
    std::cout << "  inc(): " << inc() << ", " << inc() << " (original unchanged: " << counter << ")\n";

    // 4. Generic lambda (C++14): auto params deduce types at call site
    auto multiply = [](auto a, auto b) { return a * b; };
    std::cout << "  multiply(3,4)=" << multiply(3, 4) << " multiply(2.5,3.0)=" << multiply(2.5, 3.0) << "\n";

    // 5. std::function: type-erased callable wrapper (assignable)
    std::function<int(int, int)> op = [](int a, int b) { return a + b; };
    std::cout << "  op(2,3)=" << op(2, 3) << "\n";
    op = [](int a, int b) { return a * b; };
    std::cout << "  op(2,3)=" << op(2, 3) << "\n";

    // 6. Lambdas with STL algorithms
    std::vector<int> nums = {5, 3, 8, 1, 9, 2, 7};
    std::sort(nums.begin(), nums.end(), [](int a, int b) { return a > b; }); // custom comparator
    std::cout << "  sorted desc: "; for (int n : nums) std::cout << n << " "; std::cout << "\n";
    std::cout << "  sum=" << std::accumulate(nums.begin(), nums.end(), 0) << "\n";
    std::cout << "  count>5=" << std::count_if(nums.begin(), nums.end(), [](int n) { return n > 5; }) << "\n";

    // 7. IIFE: Immediately Invoked Function Expression for scoped computation
    int result = [](int a, int b) { return a * b; }(3, 7);
    std::cout << "  IIFE(3,7)=" << result << "\n";

    // 8. Recursive lambda via std::function (for when you need recursion)
    std::function<int(int)> factorial = [&](int n) { return n <= 1 ? 1 : n * factorial(n - 1); };
    std::cout << "  factorial(5)=" << factorial(5) << "\n";
}
