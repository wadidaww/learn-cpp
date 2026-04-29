#include<bits/stdc++.h>
using namespace std;

// template<int ...Args>
// int f();



template<bool N = false, bool ...Args>
int f() {
    return (N + f<Args...>()) << 1;
}

template<bool... digits>
uint64_t reversed_binary_value()
{
    uint64_t result = 0, pos = 1;
    auto f = {(result <<= 1, result += digits)...};
    for( auto x : f) cout << x << " ";
    // auto _ = { (result += digits * pos, pos <<= 1)... };
    return result;
}



template<>
int f<false>() {
    return 0;
}

template<>
int f<true>() {
    return 1;
}

int main() {
    cout << reversed_binary_value<true, false, true, false, true>() << endl;
    return 0;
}
