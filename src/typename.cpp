#include <iostream>

template<int... Values>
struct Vector {
    static constexpr size_t size = sizeof...(Values);
    
    static void print() {
        std::cout << size << "\n";
        std::cout << "Vector<";
        bool first = true;
        ((std::cout << (first ? "" : ", ") << Values, first = false), ...);
        std::cout << ">";
    }
};

// Prefix sum calculator
template<typename InputVector, int CurrentSum = 0, typename ResultVector = Vector<>>
struct PrefixSum;

// Recursive case: process each element
template<int First, int... Rest, int CurrentSum, int... Result>
struct PrefixSum<Vector<First, Rest...>, CurrentSum, Vector<Result...>> {
    static constexpr int new_sum = CurrentSum + First;
    using type = typename PrefixSum<
        Vector<Rest...>, 
        new_sum, 
        Vector<Result..., new_sum>
    >::type;
};

// Base case: empty vector
template<int CurrentSum, int... Result>
struct PrefixSum<Vector<>, CurrentSum, Vector<Result...>> {
    using type = Vector<Result...>;
};

// Convenience alias
template<int... Values>
using PrefixSums = typename PrefixSum<Vector<Values...>>::type;

// Testing
int main() {
    // using Original = Vector<1, 2, 3, 4, 5>;
    using Result = PrefixSums<1, 2, 3, 4, 5>;
    
    // std::cout << "Original: ";
    // Original::print();
    std::cout << "\nPrefix Sums: ";
    Result::print();
    std::cout << std::endl;
    
    // Output:
    // Original: Vector<1, 2, 3, 4, 5>
    // Prefix Sums: Vector<1, 3, 6, 10, 15>
    
    return 0;
}