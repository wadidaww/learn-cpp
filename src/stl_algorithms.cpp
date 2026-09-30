// STL ALGORITHMS
// ===============
// Key concepts:
//   Non-modifying: find, count, all_of, any_of, minmax_element
//   Modifying:     transform, reverse, fill, generate, unique, copy
//   Sorting:       sort (unstable), stable_sort (stable), partial_sort, nth_element
//   Numeric:       accumulate, partial_sum, inner_product
//   Set ops:       set_union, set_intersection, set_difference (require sorted ranges)
//   Search:        binary_search, lower_bound, upper_bound (require sorted ranges)
//   Other:         next_permutation, iota, shuffle
//
// Tip: algorithms work with iterators, not containers — they work on any range

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <string>

void print(const std::string& label, const std::vector<int>& v) {
    std::cout << "  " << label << ": "; for (int i : v) std::cout << i << " "; std::cout << "\n";
}

int main() {
    // Non-modifying: check elements without changing them
    std::vector<int> nums = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3};
    std::cout << "  find(5)=" << (std::find(nums.begin(), nums.end(), 5) != nums.end()) << "\n";
    std::cout << "  count(1)=" << std::count(nums.begin(), nums.end(), 1) << "\n";
    std::cout << "  any_of(even)=" << std::any_of(nums.begin(), nums.end(), [](int n){ return n%2==0; }) << "\n";
    auto [lo, hi] = std::minmax_element(nums.begin(), nums.end());
    std::cout << "  min=" << *lo << " max=" << *hi << "\n";

    // Modifying: transform, reverse, fill, generate, unique
    std::vector<int> v = {1, 2, 3, 4, 5};
    std::vector<int> squares(v.size());
    std::transform(v.begin(), v.end(), squares.begin(), [](int n){ return n*n; });
    print("squares", squares);
    std::reverse(v.begin(), v.end()); print("reversed", v);
    std::fill(v.begin(), v.end(), 0); print("filled", v);
    std::generate(v.begin(), v.end(), [n=0]() mutable { return n++; }); print("generated", v);

    // Sorting
    std::vector<int> sortable = {5, 3, 8, 1, 9, 2, 7};
    std::sort(sortable.begin(), sortable.end()); print("sorted", sortable);
    std::sort(sortable.begin(), sortable.end(), std::greater<int>()); print("sorted desc", sortable);
    std::partial_sort(sortable.begin(), sortable.begin()+3, sortable.end()); print("partial(top3)", sortable);

    // Numeric
    std::vector<int> nums2 = {1, 2, 3, 4, 5};
    std::cout << "  accumulate=" << std::accumulate(nums2.begin(), nums2.end(), 0) << "\n";
    std::vector<int> psum(5); std::partial_sum(nums2.begin(), nums2.end(), psum.begin()); print("partial_sum", psum);

    // Set operations (require sorted ranges)
    std::vector<int> set1 = {1,2,3,4,5}, set2 = {3,4,5,6,7};
    std::vector<int> diff, uni, inter;
    std::set_difference(set1.begin(), set1.end(), set2.begin(), set2.end(), std::back_inserter(diff));
    std::set_union(set1.begin(), set1.end(), set2.begin(), set2.end(), std::back_inserter(uni));
    print("diff", diff); print("union", uni);

    // Binary search (requires sorted range)
    std::vector<int> sorted = {1,2,3,4,5,6,7,8,9,10};
    std::cout << "  binary_search(7)=" << std::binary_search(sorted.begin(), sorted.end(), 7) << "\n";

    // Permutations
    std::vector<int> perm = {1, 2, 3};
    std::cout << "  perms: ";
    do { for (int i : perm) std::cout << i; std::cout << " "; } while (std::next_permutation(perm.begin(), perm.end()));
    std::cout << "\n";

    // iota + shuffle
    std::vector<int> iota_vec(10); std::iota(iota_vec.begin(), iota_vec.end(), 1);
    std::shuffle(iota_vec.begin(), iota_vec.end(), std::mt19937{std::random_device{}()});
    print("shuffled", iota_vec);
}
