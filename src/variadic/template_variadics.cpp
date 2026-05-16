#include <functional>
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

template<typename... Ts>
constexpr auto sum_values(Ts... values) {
    return (values + ... + 0);
}

template<typename... Ts>
void print_line(const Ts&... values) {
    ((std::cout << values << ' '), ...);
    std::cout << '\n';
}

template<typename Function, typename... Args>
decltype(auto) call_with_logging(Function&& function, Args&&... args) {
    std::cout << "Calling function with " << sizeof...(Args) << " argument(s)\n";
    return std::invoke(std::forward<Function>(function), std::forward<Args>(args)...);
}

template<typename... Ts>
struct type_list {};

template<typename List>
struct list_size;

template<typename... Ts>
struct list_size<type_list<Ts...>> : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template<std::size_t Index, typename... Ts>
struct nth_type;

template<std::size_t Index, typename First, typename... Rest>
struct nth_type<Index, First, Rest...> : nth_type<Index - 1, Rest...> {};

template<typename First, typename... Rest>
struct nth_type<0, First, Rest...> {
    using type = First;
};

template<std::size_t Index, typename... Ts>
using nth_type_t = typename nth_type<Index, Ts...>::type;

template<typename Target, typename... Ts>
inline constexpr std::size_t count_type_v = (std::size_t{0} + ... + (std::is_same_v<Target, Ts> ? 1u : 0u));

template<typename... Ts>
inline constexpr bool all_integral_v = (std::is_integral_v<Ts> && ...);

template<auto... Values>
struct value_list {
    static constexpr auto sum = (Values + ... + 0);
    static constexpr auto product = (Values * ... * 1);
};

template<typename Tuple, std::size_t... Indices>
void print_tuple_impl(const Tuple& tuple, std::index_sequence<Indices...>) {
    ((std::cout << (Indices == 0 ? "" : ", ") << std::get<Indices>(tuple)), ...);
    std::cout << '\n';
}

template<typename... Ts>
void print_tuple(const std::tuple<Ts...>& tuple) {
    print_tuple_impl(tuple, std::index_sequence_for<Ts...>{});
}

int add_three_numbers(int a, int b, int c) {
    return a + b + c;
}

int main() {
    static_assert(sum_values(1, 2, 3, 4) == 10);
    static_assert(all_integral_v<int, short, long>);
    static_assert(!all_integral_v<int, double>);
    static_assert(std::is_same_v<nth_type_t<1, char, double, std::string>, double>);
    static_assert(count_type_v<int, int, double, int, char, int> == 3);
    static_assert(list_size<type_list<int, double, char>>::value == 3);
    static_assert(value_list<2, 3, 4>::sum == 9);
    static_assert(value_list<2, 3, 4>::product == 24);

    std::cout << "=== Variadic templates ===\n\n";

    std::cout << "1) Fold expressions over runtime values\n";
    std::cout << "sum_values(5, 10, 15) = " << sum_values(5, 10, 15) << "\n";
    print_line("pack contents:", 5, 10, 15, 20);
    std::cout << '\n';

    std::cout << "2) Perfect forwarding into another callable\n";
    const int total = call_with_logging(add_three_numbers, 1, 2, 3);
    std::cout << "logged total = " << total << "\n\n";

    std::cout << "3) Compile-time type computations\n";
    std::cout << "count_type_v<int, int, double, int, char, int> = "
              << count_type_v<int, int, double, int, char, int> << "\n";
    std::cout << "list_size<type_list<int, double, char>>::value = "
              << list_size<type_list<int, double, char>>::value << "\n\n";

    std::cout << "4) Compile-time value computations\n";
    std::cout << "value_list<2, 3, 4>::sum = " << value_list<2, 3, 4>::sum << "\n";
    std::cout << "value_list<2, 3, 4>::product = " << value_list<2, 3, 4>::product << "\n\n";

    std::cout << "5) Expanding packs with generated indices\n";
    print_tuple(std::make_tuple("zero", 1, 2.5, std::string("three")));
    std::cout << '\n';

    std::cout << "Rules to remember:\n";
    std::cout << "- typename... Ts captures a type pack.\n";
    std::cout << "- auto... Values captures a non-type value pack.\n";
    std::cout << "- sizeof...(pack) tells you pack length.\n";
    std::cout << "- Fold expressions replace most old recursive pack code.\n";
    std::cout << "- Use index_sequence when you need pack positions.\n";
}
