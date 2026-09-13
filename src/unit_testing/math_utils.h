#pragma once
#include <vector>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <cmath>

namespace math {

int add(int a, int b) { return a + b; }

int subtract(int a, int b) { return a - b; }

int multiply(int a, int b) { return a * b; }

double divide(double a, double b) {
    if (b == 0.0) throw std::invalid_argument("Division by zero");
    return a / b;
}

int factorial(int n) {
    if (n < 0) throw std::invalid_argument("Negative input");
    if (n <= 1) return 1;
    int result = 1;
    for (int i = 2; i <= n; ++i) result *= i;
    return result;
}

bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; ++i) {
        if (n % i == 0) return false;
    }
    return true;
}

std::vector<int> fibonacci(int count) {
    if (count <= 0) return {};
    std::vector<int> result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        if (i == 0) result.push_back(0);
        else if (i == 1) result.push_back(1);
        else result.push_back(result[i-1] + result[i-2]);
    }
    return result;
}

std::vector<int> merge_sorted(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> result;
    result.reserve(a.size() + b.size());
    std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(result));
    return result;
}

int sum_positive(const std::vector<int>& nums) {
    return std::accumulate(nums.begin(), nums.end(), 0,
        [](int sum, int n) { return n > 0 ? sum + n : sum; });
}

bool contains(const std::vector<int>& v, int val) {
    return std::find(v.begin(), v.end(), val) != v.end();
}

} // namespace math
