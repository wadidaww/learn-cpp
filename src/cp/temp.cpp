#include <algorithm>
#include <array>
#include <atomic>
#include <bits/stdc++.h>
#include <new>
#include <optional>
#include <thread>
using namespace std;

// optional<int> f() {
//     return nullopt;
// }

// struct S {
//     int x;
//     float f;
//     long long y;

//     S() {
//         static_assert(sizeof(*this) <=
//         hardware_constructive_interference_size, "S is too large"); cout <<
//         sizeof(*this) << "\n";
//     }
// };

// 27f892a4160e8ecf3429a79f6f41e4ad5d4aaaa1

atomic<int> c(0);

void func(int x) {
  for (int i = 0; i < 100; ++i) {
    cout << this_thread::get_id() << " " << (c = c + 1) << "\n";
  }
}

// class S {
//     static int x;
//     static void f() {
//         cout << x << "\n";
//     }
// };
// int S::x = 10;

// class A {
//     thread::id x;
//     static A getInstance() {
//         static A instance;
//         return instance;
//     }
// public:
//     void set(thread::id x) {
//         this->x = x;
//         cout << this->x << "\n";
//     }

//     thread::id get() {
//         return this->x;
//     }
// };

void func1(int x) {
  thread t1(func, x);
  thread t2(func, x);
  t1.join();
  t2.join();

  // auto id = this_thread::get_id();
  // thread_local A a;
  // a.set(id);
  // cout << a.get() << "\n";
}

struct A {
  template <typename T> void operator()(T arg) {
    cout << "A" << typeid(arg).name() << "\n";
  }

  float x = 0.0;

  template <typename T> float operator+(const T &&other) { return x += other; }
};

struct B {
  template <typename T> void operator()(T arg) {
    cout << "B" << typeid(arg).name() << "\n";
  }

  float x = 0.0;

  template <typename T> float operator+(const T &&other) { return x += other; }

  // partial_ordering operator<=>(const B& other) const {
  //     return x <=> other.x;
  // }
};

template <class C, class T> auto func(T val) -> void {
  C *c = new C();
  c->operator()(val);
  c->template operator()<int>(val);
  cout << c->operator+(std::move(val)) << "\n";
  cout << c->template operator+ <float>(std::move(val)) << "\n";
  cout << c->template operator+ <int>(std::move(val)) << "\n";
}

long long MOD = 998244353;

long long bin(long long a, long long b) {
  long long res = 1;
  while (b) {
    if (b & 1)
      res = (res * a) % MOD;
    a = (a * a) % MOD;
    b >>= 1;
  }
  return res;
}

int main() {
  int T;
  cin >> T;
  while (T--) {
    long long n;
    vector<long long> a;
    cin >> n;
    a.resize(n);
    for (int i = 0; i < n; ++i) {
      cin >> a[i];
    }
    sort(a.begin(), a.end());
    long long cnt = 0;
    bool equal = false;
    equal = true;
    for (int i = 0; i < n; ++i) {
      if (a[0] != a[i]) {
        equal = false;
        break;
      }
    }
    while (!equal && a[0] > 2) {
      for (int i = 1; i < n; ++i) {
        while (a[i] > 2 && a[i] != a[0]) {
          if (a[i] & 1) {
            if (a[i] == a[0])
              break;
            if (a[i] + 1 < a[0])
              break;
            a[i] += 1;
          } else {
            if (a[i] <= a[0])
              break;
            if (a[i] - 1 == a[0])
              break;
            a[i] /= 2;
          }
          ++cnt;
        }
      }
      equal = true;
      for (int i = 0; i < n; ++i) {
        if (a[0] != a[i]) {
          equal = false;
          break;
        }
      }
      if (equal)
        break;
      if (a[0] & 1) {
        a[0] += 1;
      } else {
        a[0] /= 2;
      }
      ++cnt;
      equal = true;
      for (int i = 0; i < n; ++i) {
        if (a[0] != a[i]) {
          equal = false;
          break;
        }
      }
    }
    long long mn = 0;
    if (a[0] <= 2) {
      for (int i = 0; i < n; ++i) {
        while (a[i] > 2) {
          if (a[i] & 1) {
            a[i] += 1;
          } else {
            a[i] /= 2;
          }
          ++cnt;
        }
      }
      long long cnt[2] = {0, 0};
      for (int i = 0; i < n; ++i) {
        if (a[i] == 1) {
          ++cnt[0];
        } else if (a[i] == 2) {
          ++cnt[1];
        }
      }
      mn = min(cnt[0], cnt[1]);
    }
    cout << cnt + mn << "\n";
  }
  return 0;
  func1(5);
  // int arr[10][2][2][4];
  // auto arr2 = std::to_array(arr);
  // std::optional<long long> o;
  // cout << sizeof(o) << "\n";
  // cout << sizeof(int) << "\n";
  // pair<int, int> p;
  // pair<int, bool> q;
  // cout << sizeof(pair<int, int>) << " " << sizeof(pair<int, bool>) << "\n";
  // cout << sizeof(pair<long long, int>) << " " << sizeof(pair<long long,
  // bool>) << "\n"; std::nullopt; nullptr_t null = nullptr;

  // cout << sizeof(nullopt) << " " << sizeof(nullopt_t) << " " <<
  // sizeof(nullptr) << " " << sizeof(nullptr_t) << "\n"; enum class E { one,
  // two }; cout << sizeof(E) << "\n";

  // return 0;

  // char *str = "H";
  // char *str2 = "Hello";
  // str[1] = 'a';
  // str[2] = '\0';
  // why does it cause segmentation fault?
  // It causes a segmentation fault because string literals are stored in
  // read-only memory. When you try to modify the string literal by assigning a
  // new value to str[1] and str[2], you are trying to write to read-only
  // memory, which is not allowed and results in a segmentation fault. cout <<
  // str << "\n"; cout << str2 << "\n"; str = str2; cout << str << "\n"; cout <<
  // str2 << "\n"; cout << strlen(str) << "\n"; how does it know the size of the
  // string literal? It doesn't, it just treats it as a pointer to the first
  // character. The size of the string literal is determined at compile time and
  // is not stored in the pointer. The sizeof operator will return the size of
  // the pointer, not the size of the string literal. cout << sizeof(*str) <<
  // "\n"; int *a = new int(5); int *c = &*a; cout << (a == c) << "\n"; if (int
  // x = 1-2; x < 0) {
  //     cout << "x is negative\n";
  // } else {
  //     cout << "x is non-negative\n";
  // }
  // float x = 1.1;
  // float b = 1.2;
  // func<A>(x);
  // func<B>(b);
  // thread t1(func1);
  // thread t2(func1);

  // std::array<int, 5> arr({1, 2, 3, 4, 5});
  // cout << std::any_of(arr.begin(), arr.end(), [](int x) { return x % 2 == 0;
  // }) << "\n";

  // thread t1(func);
  // thread t2(func);
  // t1.join();
  // t2.join();
  // cout << c.load() << "\n";
  // S();
  // vector<int> vec;
  // vec.resize(10);
  // cout << vec.capacity() << endl;
  // vec.clear();
  // cout << vec.capacity() << endl;
  // vec = vector<int>();
  // cout << vec.capacity() << endl;
  // new int[1000];
  return 0;
}