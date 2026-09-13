#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include <string>
#include <random>
#include <ranges>
#include <execution>
#include <iterator>
#include <set>

void print_vec(const std::string& label, const std::vector<int>& v) {
    std::cout << "  " << label << ": ";
    for (int i : v) std::cout << i << " ";
    std::cout << "\n";
}

struct Student {
    std::string name;
    int grade;
};

int main() {
    // --- 1. Non-modifying algorithms ---
    std::cout << "=== 1. Non-Modifying Algorithms ===\n";
    std::vector<int> nums = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};

    auto it = std::find(nums.begin(), nums.end(), 5);
    if (it != nums.end()) print_vec("find(5)", {static_cast<int>(it - nums.begin())});

    int cnt = std::count(nums.begin(), nums.end(), 1);
    std::cout << "  count(1) = " << cnt << "\n";

    bool any_even = std::any_of(nums.begin(), nums.end(), [](int n) { return n % 2 == 0; });
    std::cout << "  any_of(even) = " << std::boolalpha << any_even << "\n";

    bool all_positive = std::all_of(nums.begin(), nums.end(), [](int n) { return n > 0; });
    std::cout << "  all_of(positive) = " << all_positive << "\n";

    auto [min_it, max_it] = std::minmax_element(nums.begin(), nums.end());
    std::cout << "  min=" << *min_it << " max=" << *max_it << "\n";

    // --- 2. Modifying algorithms ---
    std::cout << "\n=== 2. Modifying Algorithms ===\n";
    std::vector<int> v1 = {1, 2, 3, 4, 5};
    std::vector<int> v2(v1.size());

    std::transform(v1.begin(), v1.end(), v2.begin(), [](int n) { return n * n; });
    print_vec("squares", v2);

    std::reverse(v1.begin(), v1.end());
    print_vec("reversed", v1);

    std::rotate(v1.begin(), v1.begin() + 2, v1.end());
    print_vec("rotate(2)", v1);

    std::fill(v1.begin(), v1.end(), 0);
    print_vec("filled 0", v1);

    std::generate(v1.begin(), v1.end(), [n = 0]() mutable { return n++; });
    print_vec("generated", v1);

    auto last = std::unique(v1.begin(), v1.end());
    v1.erase(last, v1.end());
    print_vec("unique", v1);

    std::vector<int> src = {10, 20, 30};
    std::vector<int> dst(3);
    std::copy(src.begin(), src.end(), dst.begin());
    print_vec("copied", dst);

    // --- 3. Sorting ---
    std::cout << "\n=== 3. Sorting ===\n";
    std::vector<int> sortable = {5, 3, 8, 1, 9, 2, 7, 4, 6};
    std::sort(sortable.begin(), sortable.end());
    print_vec("sorted asc", sortable);

    std::sort(sortable.begin(), sortable.end(), std::greater<int>());
    print_vec("sorted desc", sortable);

    // Partial sort
    std::partial_sort(sortable.begin(), sortable.begin() + 3, sortable.end());
    print_vec("partial_sort (top 3)", sortable);

    // Nth element
    std::nth_element(sortable.begin(), sortable.begin() + 4, sortable.end());
    std::cout << "  median (nth=4): " << sortable[4] << "\n";

    // Stable sort with custom key
    std::vector<Student> students = {
        {"Alice", 85}, {"Bob", 90}, {"Charlie", 85}, {"Diana", 90}
    };
    std::stable_sort(students.begin(), students.end(),
        [](const Student& a, const Student& b) { return a.grade > b.grade; });
    std::cout << "  sorted by grade desc:\n";
    for (const auto& s : students)
        std::cout << "    " << s.name << ": " << s.grade << "\n";

    // --- 4. Numeric algorithms ---
    std::cout << "\n=== 4. Numeric Algorithms ===\n";
    std::vector<int> nums2 = {1, 2, 3, 4, 5};
    int sum = std::accumulate(nums2.begin(), nums2.end(), 0);
    std::cout << "  accumulate = " << sum << "\n";

    std::vector<int> nums3 = {1, 2, 3, 4, 5};
    std::vector<int> result(5);
    std::partial_sum(nums2.begin(), nums2.end(), result.begin());
    print_vec("partial_sum", result);

    int inner = std::inner_product(nums2.begin(), nums2.end(), nums3.begin(), 0);
    std::cout << "  inner_product = " << inner << "\n";

    auto [lo, hi] = std::minmax_element(nums2.begin(), nums2.end());
    std::cout << "  range: [" << *lo << ", " << *hi << "]\n";

    // --- 5. Set operations ---
    std::cout << "\n=== 5. Set Operations ===\n";
    std::vector<int> set1 = {1, 2, 3, 4, 5};
    std::vector<int> set2 = {3, 4, 5, 6, 7};
    std::vector<int> diff, uni, inter;

    std::set_difference(set1.begin(), set1.end(), set2.begin(), set2.end(),
        std::back_inserter(diff));
    print_vec("set1 - set2", diff);

    std::set_union(set1.begin(), set1.end(), set2.begin(), set2.end(),
        std::back_inserter(uni));
    print_vec("union", uni);

    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
        std::back_inserter(inter));
    print_vec("intersection", inter);

    // --- 6. Binary search ---
    std::cout << "\n=== 6. Binary Search ===\n";
    std::vector<int> sorted = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    bool found = std::binary_search(sorted.begin(), sorted.end(), 7);
    std::cout << "  binary_search(7) = " << found << "\n";

    auto lb = std::lower_bound(sorted.begin(), sorted.end(), 5);
    auto ub = std::upper_bound(sorted.begin(), sorted.end(), 5);
    std::cout << "  lower_bound(5) index: " << (lb - sorted.begin()) << "\n";
    std::cout << "  upper_bound(5) index: " << (ub - sorted.begin()) << "\n";

    // --- 7. Permutations ---
    std::cout << "\n=== 7. Permutations ===\n";
    std::vector<int> perm = {1, 2, 3};
    int count = 0;
    do {
        std::cout << "  perm " << ++count << ": ";
        for (int i : perm) std::cout << i;
        std::cout << "\n";
    } while (std::next_permutation(perm.begin(), perm.end()));

    // --- 8. iota + algorithms ---
    std::cout << "\n=== 8. iota + Algorithms ===\n";
    std::vector<int> iota_vec(10);
    std::iota(iota_vec.begin(), iota_vec.end(), 1);
    print_vec("iota(1..10)", iota_vec);

    std::shuffle(iota_vec.begin(), iota_vec.end(),
        std::mt19937{std::random_device{}()});
    print_vec("shuffled", iota_vec);

    std::cout << "\nDone.\n";
    return 0;
}
