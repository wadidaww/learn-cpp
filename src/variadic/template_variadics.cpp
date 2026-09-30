#include <functional>
#include <stdexcept>
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

template<typename... Ts>
constexpr auto sum_values(Ts... values) {
    return (values + ... + 0);
}

template<typename T>
constexpr auto min_value(T value) {
    return value;
}

template<typename First, typename Second, typename... Rest>
constexpr auto min_value(First first, Second second, Rest... rest) {
    using Value = std::common_type_t<First, Second, Rest...>;
    Value minimum = static_cast<Value>(first);
    const auto keep_smaller = [&minimum](const auto& value) {
        const Value candidate = static_cast<Value>(value);
        if (candidate < minimum) {
            minimum = candidate;
        }
    };
    keep_smaller(second);
    (keep_smaller(rest), ...);
    return minimum;
}

template<typename First, typename... Rest>
auto make_vector(First&& first, Rest&&... rest) {
    using Value = std::common_type_t<First, Rest...>;
    std::vector<Value> result;
    result.reserve(sizeof...(Rest) + 1);
    result.emplace_back(std::forward<First>(first));
    (result.emplace_back(std::forward<Rest>(rest)), ...);
    return result;
}

template<typename... Ts>
void print_line(const Ts&... values) {
    ((std::cout << values << ' '), ...);
    std::cout << '\n';
}

void tiny_printf_impl(std::ostream& output, const std::string& format, std::size_t offset) {
    // Base case for the recursive pack expansion.
    const std::size_t placeholder = format.find("{}", offset);
    if (placeholder != std::string::npos) {
        throw std::invalid_argument("tiny_printf received fewer values than placeholders");
    }

    output << format.substr(offset);
}

template<typename First, typename... Rest>
void tiny_printf_impl(std::ostream& output, const std::string& format, std::size_t offset, First&& first, Rest&&... rest) {
    const std::size_t placeholder = format.find("{}", offset);
    if (placeholder == std::string::npos) {
        throw std::invalid_argument("tiny_printf received more values than placeholders");
    }

    output << format.substr(offset, placeholder - offset);
    output << std::forward<First>(first);
    tiny_printf_impl(output, format, placeholder + 2, std::forward<Rest>(rest)...);
}

template<typename... Ts>
void tiny_printf(std::ostream& output, const std::string& format, Ts&&... values) {
    tiny_printf_impl(output, format, 0, std::forward<Ts>(values)...);
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
    static_assert(min_value(7, 4, 9, 2, 5) == 2);
    static_assert(all_integral_v<int, short, long>);
    static_assert(!all_integral_v<int, double>);
    static_assert(std::is_same_v<nth_type_t<1, char, double, std::string>, double>);
    static_assert(count_type_v<int, int, double, int, char, int> == 3);
    static_assert(std::is_same_v<typename decltype(make_vector(1, 2.5, 3u))::value_type, double>);
    static_assert(list_size<type_list<int, double, char>>::value == 3);
    static_assert(value_list<2, 3, 4>::sum == 9);
    static_assert(value_list<2, 3, 4>::product == 24);

    std::cout << "=== Variadic templates ===\n\n";

    std::cout << "1) Fold expressions over runtime values\n";
    std::cout << "sum_values(5, 10, 15) = " << sum_values(5, 10, 15) << "\n";
    std::cout << "min_value(7, 4, 9, 2, 5) = " << min_value(7, 4, 9, 2, 5) << "\n";
    print_line("pack contents:", 5, 10, 15, 20);
    std::cout << '\n';

    std::cout << "2) Deducing one container type from a pack\n";
    const auto values = make_vector(1, 2.5, 3u);
    std::cout << "make_vector(1, 2.5, 3u) stores " << values.size()
              << " values with common type double: ";
    for (const auto value : values) {
        std::cout << value << ' ';
    }
    std::cout << "\n\n";

    std::cout << "3) Tiny printf-style formatting with a type-safe argument pack\n";
    tiny_printf(std::cout, "name={}, score={}, active={}\n\n", "Ada", 98.5, true);

    std::cout << "4) Perfect forwarding into another callable\n";
    const int total = call_with_logging(add_three_numbers, 1, 2, 3);
    std::cout << "logged total = " << total << "\n\n";

    std::cout << "5) Compile-time type computations\n";
    std::cout << "count_type_v<int, int, double, int, char, int> = "
              << count_type_v<int, int, double, int, char, int> << "\n";
    std::cout << "nth_type_t<1, char, double, std::string> is double = "
              << std::boolalpha << std::is_same_v<nth_type_t<1, char, double, std::string>, double>
              << std::noboolalpha << "\n";
    std::cout << "list_size<type_list<int, double, char>>::value = "
              << list_size<type_list<int, double, char>>::value << "\n\n";

    std::cout << "6) Compile-time value computations\n";
    std::cout << "value_list<2, 3, 4>::sum = " << value_list<2, 3, 4>::sum << "\n";
    std::cout << "value_list<2, 3, 4>::product = " << value_list<2, 3, 4>::product << "\n\n";

    std::cout << "7) Expanding packs with generated indices\n";
    print_tuple(std::make_tuple("zero", 1, 2.5, std::string("three")));
    std::cout << '\n';

    std::cout << "Rules to remember:\n";
    std::cout << "- typename... Ts captures a type pack.\n";
    std::cout << "- auto... Values captures a non-type value pack.\n";
    std::cout << "- sizeof...(pack) tells you pack length.\n";
    std::cout << "- Fold expressions replace most old recursive pack code.\n";
    std::cout << "- Use index_sequence when you need pack positions.\n";
}
