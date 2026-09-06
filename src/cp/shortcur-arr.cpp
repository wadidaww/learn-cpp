#include <bits/stdc++.h>
using namespace std;

template <class T, size_t... Rest> struct MyArray;

template <class T> struct MyArray<T> {
  using type = T;
};

template <size_t N, size_t... Rest> struct MyArray {
  using type = array<MyArray<Rest...>::type, N>;
};

template <class T, size_t N, size_t... Rest> struct MyArray {
  using type = MyArray<std::array<T, N>, Rest...>;
};

template <int... Rest> struct S;

template <> struct S<> {
  static const int val = 0;
};

template <int N, int... Rest> struct S<N, Rest...> {
  static const int val = N + S<Rest...>::val;
};

int main() { cout << S<1, 2, 10>::val << endl; }