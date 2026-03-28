// // #include<bits/stdc++.h>
// // #include <cstddef>
// // #include <thread>
// // using namespace std;

// // // long long n, d, s;

// // // bool valid(long long val) {
// // //     long long cur = val / s;
// // //     vector<long long> fs;
// // //     for (long long x = 2 ; x <= cur / x ; ++x) {
// // //         while (cur % x == 0) {
// // //             cur /= x; 
// // //             fs.push_back(x);
// // //         }
// // //     }
// // //     fs.push_back(cur);
// // //     sort(fs.begin(), fs.end());
// // //     reverse(fs.begin(), fs.end());
// // //     long long x = s;
// // //     for(auto f : fs) {
// // //         long long y = x * f;
// // //         if (y - x > d) return false;
// // //         // cout << x << " " << y << endl;
// // //         x = y;
// // //     }
// // //     return true;
// // // }
// // // 60
// // // 2 2 3 5
// // // 3 -> 15 -> 30 -> 60


// // class Class {
// //     private:
// //         int a;
// //     public:
// //         Class(int a) : a(a) {}
// //         int getA() const { return a; }
// //     friend std::ostream& operator<<(std::ostream& os, const Class& obj) {
// //         os << obj.a;
// //         return os;
// //     }
// //     friend std::istream& operator>>(std::istream& is, Class& obj) {
// //         is >> obj.a;
// //         return is;
// //     }
// // };

// // struct PointUnaligned {
// //     double x;
// //     double y;
// // };

// // // Define a Point structure with cache line alignment
// // struct __attribute__((aligned(16))) PointAligned {
// //     double x;
// //     double y;
// // };


// // // void f1() {
// // // for(size_t i = 0 ; i < 100 ; ++i) {

// // //     cout << "a";
// // //     this_thread::sleep_for(std::chrono::milliseconds(1));
// // // }
// // // }

// // // int x;
// // // int z;
// // // sizeof(Base) = 16 (vptr 8 + int 4 + padding 4) on 64-bit

// // //

// // class Base {
// // public:
// // char d[5];
// // virtual void f(); // 8
// // };
// // class Derived : public Base { // Base + x
// // public:
// //     void f() override {}
// //     char c[3];
// // };
// //  // still one vptr, size = 24 (Base 16 + y 4 + padding 4)
// // // int k;
// // // int p;
// // // int m;
// // // int q;

// // // struct A { virtual void f(); int a; }; // vptr + int = 16 (64-bit), 8 + 4 + padding 4
// // // struct B : virtual A {};       // vptr to A + b + padding = 32 (approx)
// // // struct C : virtual A { int c; };       // similarly 32
// // // struct D : B, C { int d; };            // B's non-virt part, C's non-virt part, D's members, shared A

// // template<typename ...Args>
// // void print(Args&&... args) {
// //     ((cout << &args << " "), ...);
// //     cout << "\n";
// // }

// // template<int... Values>
// // struct Vector {
// //     static constexpr size_t size = sizeof...(Values);
    
// //     static void print() {
// //         std::cout << "Vector<";
// //         bool first = true;
// //         ((std::cout << (first ? "" : ", ") << Values, first = false), ...);
// //         std::cout << ">";
// //     }
// // };

// // // Prefix sum calculator
// // template<typename InputVector, int CurrentSum = 0, typename ResultVector = Vector<>>
// // struct PrefixSum;

// // // Recursive case: process each element
// // template<int First, int... Rest, int CurrentSum, int... Result>
// // struct PrefixSum<Vector<First, Rest...>, CurrentSum, Vector<Result...>> {
// //     static constexpr int new_sum = CurrentSum + First;
// //     using type = typename PrefixSum<
// //         Vector<Rest...>, 
// //         new_sum, 
// //         Vector<Result..., new_sum>
// //     >::type;
// // };

// // // Base case: empty vector
// // template<int CurrentSum, int... Result>
// // struct PrefixSum<Vector<>, CurrentSum, Vector<Result...>> {
// //     using type = Vector<Result...>;
// // };

// // // Convenience alias
// // template<int... Values>
// // using PrefixSums = typename PrefixSum<Vector<Values...>>::type;

// // // prefix sum with template
// // template <int X, int&...Args>
// // void prefix_sum() {
// //     return (X + ... + Args);
// // }

// // template <int X>
// // int prefix_sum() {
// //     return X;
// // }


// // int main() {
// //     // using Original = Vector<1, 2, 3, 4, 5>;
// //     using Result = PrefixSums<1, 2, 3, 4, 5>;
    
// //     // std::cout << "Original: ";
// //     // Original::print();
// //     std::cout << "\nPrefix Sums: ";
// //     Result::print();
// //     std::cout << std::endl;
    
// //     // Output:
// //     // Original: Vector<1, 2, 3, 4, 5>
// //     // Prefix Sums: Vector<1, 3, 6, 10, 15>
    
// //     return 0;
// //     // cout << sizeof(A) << "\n";
// //     // cout << sizeof(B) << "\n";
// //     // cout << sizeof(C) << "\n";
// //     // cout << sizeof(D) << "\n";
// //     // cout << offsetof(Base, d) << "\n";
// //     // cout << sizeof(Base) << "\n";
// //     // cout << sizeof(Derived) << "\n";
// //     // std::thread t1(f1);
// //     // std::thread t2([]{for(size_t i = 0 ; i < 100 ; ++i){
// //     //     cout << "b";
// //     //     // delay 1ms
// //     //     std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
// //     // }});
// //     // t1.join();
// //     // t2.join();
// //     // cout << "\n";
// //     // PointAligned pa;
// //     // PointUnaligned pu;
// //     // cout << sizeof(PointAligned) << "\n";
// //     // cout << sizeof(PointUnaligned) << "\n";
// //     // float a = 0.1;
// //     // int b;
// //     // std::memcpy(&b, &a, sizeof(float));
// //     // cout << b << endl << bitset<32>(b) << endl;
// //     // a += 0.4;
// //     // std::memcpy(&b, &a, sizeof(float));
// //     // cout << b << endl << bitset<32>(b) << endl;
// //     return 0;



// //     // std::atomic<int> a(0);
// //     // a.fetch_add(10);
// //     // a.store(1);
// //     // cout << a.load();
// //     // Class c(5);
// //     // cin >> c;
// //     // cout << c << endl;
// //     // cout << c.getA() << endl;
// //     // cout << c.getA() << endl;
// //     // if (1 == 2) {

// //     // }

// //     // float a = 1;
// //     // unsigned long l;
// //     // memcpy(&l, &a, sizeof(float));
// //     // bitset<32> b(*reinterpret_cast<unsigned long*>(&a));
// //     // cout << (b  == 0b00111111100000000000000000000000) << endl;
// //     // cout << (l  == 0b00111111100000000000000000000000) << endl;
// //     // cout << l << endl;

// //     // vector<int> vec(10000000, 1);
// //     // int cur = vec[0];
// //     // for(size_t i = 1 ; i < vec.size() ; ++i) {
// //     //     cur += vec[i];
// //     //     vec[i] = cur;
// //     // }
// //     // cout << vec.back() << endl;
// //     // sort(vec.begin(), vec.end());
// //     // cin >> n >> d >> s;

// //     // long long ans = min(n, d * 2);
// //     // ans /= s;
// //     // ans *= s;
// //     // while(ans >= s * 2) {
// //     //     if (valid(ans)) {
// //     //         cout << ans << endl;
// //     //         return 0;
// //     //     }
// //     //     ans -= s;
// //     // }
// //     // cout << s << endl;
// //     return 0;
// // }
// // // 1000000000000
// // // 1000000000
// // // 9999999999

// // // 1000000000000
// // // 100000000000
// // // 10000000001

// // // 1000000000000
// // // 100000000000
// // // 10000000001


// // #include <atomic>
// // #include <iostream>

// // struct A
// // {
// //     virtual void asdf() {}
// //     char qmChar; // size: 1
// // public:
// //     virtual void qasdf() {}
// //     int mChar; // size: 1
// //     virtual void f(){}
// //     char amChar; // size: 1
// //     // double mInt; // size: 4
// // };

// // struct B 
// // {
// //     char mChar; // size: 1
// //     double mDouble; // size: 8
// //     int mInt; // size: 4
// // };

// // struct C
// // {
// //     double mDouble; // size: 8
// //     int mInt; // size: 4
// //     char mChar; // size: 1
// // };
 
// // int main()
// // {

// //     std::atomic<int> a(11);
// //     int d = 10;
// //     int c = 1;

// //     std::cout << a.compare_exchange_strong(d, c) << "\n";
// //     std::cout << a.load() << " " << d << " " << c << "\n";

// //     std::cout << "Size of struct A is: " << sizeof(A)
// //         << " with alignment: " << alignof(A) << std::endl;

// //     std::cout << "Size of struct B is: " << sizeof(B)
// //         << " with alignment: " << alignof(B) << std::endl;

// //     std::cout << "Size of struct C is: " << sizeof(C)
// //         << " with alignment: " << alignof(C) << std::endl;

// //     return 0;
// // }

// // List node
// // #include <atomic>
// // struct Node
// // {
// //     int mValue{0};
// //     Node* mNext{nullptr};
// // };

// // std::atomic<Node*> pHead(nullptr);

// // Push a node to the front of the list
// // void PushFront(const int& value)
// // {
// //     Node* newNode = new Node();
// //     newNode->mValue = value;
// //     Node* expected = pHead.load();
   
// //     // Try push the node we have just created to the front of the list
// //     do
// //     {
// //         newNode->mNext = pHead;
// //     }
// //     while(!pHead.compare_exchange_strong(expected, newNode));
// // }

// // #include <array>
// // #include <atomic>
// // #include <iostream>
// // #include <mutex>
// // #include <vector>
// // using namespace std;

// // template <class C>
// // struct B {
// //     void f() {
// //         return static_cast<C*>(this)->fImpl();
// //     }
// // };

// // struct S : public B<S> {
// //     S(int x):a(x){}
// //     int a;
// //     // virtual void fImpl() {}
// // };

// // struct A {
// //     char c[4];
// // };

// #include<bits/stdc++.h>
// #include <queue>
// #include <vector>
// using namespace std;
// struct cmp {
//     bool operator()(const int& a, const int& b) const {
//         return a < b;
//     }
//     void operator()(const int& a) const {
//         cout << a << endl;
//     }
// };

// bool gg(int a, int b) {
//     return a < b;
// }

// inline bool findEl(int x, const vector<int> &v) {
//     for(int i : v) {
//         if (i == x) return true;
//     }
//     return false;
// }

// // inline bool findEl(int x, vector<int> &v) {
// //     if (v.empty()) return false;
// //     if (v.back() == x) return true;
// //     v.pop_back();
// //     if (findEl(x, v)) return true;
// //     return false;
// // }
// inline int add(int a, int b) { return a + b; }


// int main() {

//     int (*ptr)(int, int) = add;   // address taken → compiler must emit standalone copy
//     ptr(2, 3);                     // call via pointer → cannot inline

//     vector<int> ve = {5, 1, 3};
//     cout << findEl(1, ve) << endl;
//     cmp a;
//     a(123213);
//     vector<int> v = {5, 1, 3};
//     sort(v.begin(), v.end(), gg);
//     for (int x : v) {
//         cout << x << " ";
//     }
//     cout << endl;
//     std::priority_queue<int, vector<int>, cmp> pq;
//     pq.push(5);
//     pq.push(1);
//     pq.push(3);
//     while (!pq.empty()) {
//         cout << pq.top() << " ";
//         pq.pop();
//     }
//     cout << endl;
//     // mutex mtx;
//     // mtx.lock();
//     // A a;
//     // cout << a.c << endl;
//     // mtx.unlock();
//     // std::atomic<S> s(4);
//     // s.store(0);
//     // std::cout << s.load().a << std::endl;
//     // std::atomic<std::array<int, 5>> ar;
//     // std::atomic<std::vector<int>> a;
//     return 0;
// }

#include <atomic>
#include <bits/stdc++.h>
#include <cstdlib>
#include <memory>
#include <new>
#include <type_traits>
#include <typeindex>
#include <utility>
using namespace std;

void print_type(int &&param) {
    cout << "Rvalue reference\n";
    // Helper to print type name (compiler-specific output)
    std::cout << "Deduced T: " << typeid(int).name() << '\n'; 
}

void print_type(int &param) {
    cout << "Lvalue reference\n";
    // Helper to print type name (compiler-specific output)
    std::cout << "Deduced T: " << typeid(int).name() << '\n'; 
}
 
template <typename T>
void func(T &&param) { // T is deduced from the argument
    print_type(std::forward<T>(param)); // Forward the parameter to preserve its value category
    // print_type(static_cast<T&&>(param)); // Forward the parameter to preserve its value category
}



template<typename Target, typename ...T>
Target myMax(Target cur, T &&...params) {
    static_assert(sizeof...(params) > 0, "At least one argument is required");
    // assert all T elements are convertible to Target
    static_assert((std::is_convertible_v<T, Target> && ...), "All parameters must be convertible to Target");
    Target ans = cur;
    ((ans = max(ans, static_cast<Target>(params))), ...);
    return ans;
}

void fun(const int *y) {
    // static const int x = 42;
    // constexpr int cst = 10;
    // auto x = (int*)malloc(sizeof(int));
    // *x = 42;
    // memcpy((void*)y, &x, sizeof(int));
    y = new int(42);
}

class C {
public:
    alignas(hardware_constructive_interference_size) int x;
    C(int y):x(y) {}

    C operator=(C &c)= delete; 
    C operator=(C c)= delete; 
    // C operator=(C &&c)= delete; 

    // ~C() {
    //     delete &x;
    // }
};

C get(const int **y) {
    // C a = *new C(11);
    // *y = &a.x;
    // return *new C(11);
    return C(11);
    // return x;
}

C get2(const int **y) {
    // return C(12);
    auto x = C(12);
    return x;
}

C get3(unique_ptr<int> ptr) {
    return C(1);
}


class B {
public:
    inline virtual void f() {
        cout << "Asdf";
    }
};

class A: public B {
public:
    inline void f() override {
        cout << "qwer";
    }
};

// template<typename T>
// class SmartPtr {
//     int *cnt;
//     T *ptr;
// public:
//     SmartPtr():cnt(new int(0)), ptr(nullptr) {}
//     SmartPtr(T *obj):cnt(new int(1)) {ptr = obj;}
//     // SmartPtr(T *obj):cnt(new int(1)) {ptr = obj;}
//     ~SmartPtr() {
//         if (--*cnt == 0) {
//             delete ptr;
//             delete cnt;
//         }
//     }
// };

// // #include<bits/stdc++.h>
// // #include <cstddef>
// // #include <thread>
// // using namespace std;

// // // long long n, d, s;

// // // bool valid(long long val) {
// // //     long long cur = val / s;
// // //     vector<long long> fs;
// // //     for (long long x = 2 ; x <= cur / x ; ++x) {
// // //         while (cur % x == 0) {
// // //             cur /= x; 
// // //             fs.push_back(x);
// // //         }
// // //     }
// // //     fs.push_back(cur);
// // //     sort(fs.begin(), fs.end());
// // //     reverse(fs.begin(), fs.end());
// // //     long long x = s;
// // //     for(auto f : fs) {
// // //         long long y = x * f;
// // //         if (y - x > d) return false;
// // //         // cout << x << " " << y << endl;
// // //         x = y;
// // //     }
// // //     return true;
// // // }
// // // 60
// // // 2 2 3 5
// // // 3 -> 15 -> 30 -> 60


// // class Class {
// //     private:
// //         int a;
// //     public:
// //         Class(int a) : a(a) {}
// //         int getA() const { return a; }
// //     friend std::ostream& operator<<(std::ostream& os, const Class& obj) {
// //         os << obj.a;
// //         return os;
// //     }
// //     friend std::istream& operator>>(std::istream& is, Class& obj) {
// //         is >> obj.a;
// //         return is;
// //     }
// // };

// // struct PointUnaligned {
// //     double x;
// //     double y;
// // };

// // // Define a Point structure with cache line alignment
// // struct __attribute__((aligned(16))) PointAligned {
// //     double x;
// //     double y;
// // };


// // // void f1() {
// // // for(size_t i = 0 ; i < 100 ; ++i) {

// // //     cout << "a";
// // //     this_thread::sleep_for(std::chrono::milliseconds(1));
// // // }
// // // }

// // // int x;
// // // int z;
// // // sizeof(Base) = 16 (vptr 8 + int 4 + padding 4) on 64-bit

// // //

// // class Base {
// // public:
// // char d[5];
// // virtual void f(); // 8
// // };
// // class Derived : public Base { // Base + x
// // public:
// //     void f() override {}
// //     char c[3];
// // };
// //  // still one vptr, size = 24 (Base 16 + y 4 + padding 4)
// // // int k;
// // // int p;
// // // int m;
// // // int q;

// // // struct A { virtual void f(); int a; }; // vptr + int = 16 (64-bit), 8 + 4 + padding 4
// // // struct B : virtual A {};       // vptr to A + b + padding = 32 (approx)
// // // struct C : virtual A { int c; };       // similarly 32
// // // struct D : B, C { int d; };            // B's non-virt part, C's non-virt part, D's members, shared A

// // template<typename ...Args>
// // void print(Args&&... args) {
// //     ((cout << &args << " "), ...);
// //     cout << "\n";
// // }

// // template<int... Values>
// // struct Vector {
// //     static constexpr size_t size = sizeof...(Values);
    
// //     static void print() {
// //         std::cout << "Vector<";
// //         bool first = true;
// //         ((std::cout << (first ? "" : ", ") << Values, first = false), ...);
// //         std::cout << ">";
// //     }
// // };

// // // Prefix sum calculator
// // template<typename InputVector, int CurrentSum = 0, typename ResultVector = Vector<>>
// // struct PrefixSum;

// // // Recursive case: process each element
// // template<int First, int... Rest, int CurrentSum, int... Result>
// // struct PrefixSum<Vector<First, Rest...>, CurrentSum, Vector<Result...>> {
// //     static constexpr int new_sum = CurrentSum + First;
// //     using type = typename PrefixSum<
// //         Vector<Rest...>, 
// //         new_sum, 
// //         Vector<Result..., new_sum>
// //     >::type;
// // };

// // // Base case: empty vector
// // template<int CurrentSum, int... Result>
// // struct PrefixSum<Vector<>, CurrentSum, Vector<Result...>> {
// //     using type = Vector<Result...>;
// // };

// // // Convenience alias
// // template<int... Values>
// // using PrefixSums = typename PrefixSum<Vector<Values...>>::type;

// // // prefix sum with template
// // template <int X, int&...Args>
// // void prefix_sum() {
// //     return (X + ... + Args);
// // }

// // template <int X>
// // int prefix_sum() {
// //     return X;
// // }


// // int main() {
// //     // using Original = Vector<1, 2, 3, 4, 5>;
// //     using Result = PrefixSums<1, 2, 3, 4, 5>;
    
// //     // std::cout << "Original: ";
// //     // Original::print();
// //     std::cout << "\nPrefix Sums: ";
// //     Result::print();
// //     std::cout << std::endl;
    
// //     // Output:
// //     // Original: Vector<1, 2, 3, 4, 5>
// //     // Prefix Sums: Vector<1, 3, 6, 10, 15>
    
// //     return 0;
// //     // cout << sizeof(A) << "\n";
// //     // cout << sizeof(B) << "\n";
// //     // cout << sizeof(C) << "\n";
// //     // cout << sizeof(D) << "\n";
// //     // cout << offsetof(Base, d) << "\n";
// //     // cout << sizeof(Base) << "\n";
// //     // cout << sizeof(Derived) << "\n";
// //     // std::thread t1(f1);
// //     // std::thread t2([]{for(size_t i = 0 ; i < 100 ; ++i){
// //     //     cout << "b";
// //     //     // delay 1ms
// //     //     std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
// //     // }});
// //     // t1.join();
// //     // t2.join();
// //     // cout << "\n";
// //     // PointAligned pa;
// //     // PointUnaligned pu;
// //     // cout << sizeof(PointAligned) << "\n";
// //     // cout << sizeof(PointUnaligned) << "\n";
// //     // float a = 0.1;
// //     // int b;
// //     // std::memcpy(&b, &a, sizeof(float));
// //     // cout << b << endl << bitset<32>(b) << endl;
// //     // a += 0.4;
// //     // std::memcpy(&b, &a, sizeof(float));
// //     // cout << b << endl << bitset<32>(b) << endl;
// //     return 0;



// //     // std::atomic<int> a(0);
// //     // a.fetch_add(10);
// //     // a.store(1);
// //     // cout << a.load();
// //     // Class c(5);
// //     // cin >> c;
// //     // cout << c << endl;
// //     // cout << c.getA() << endl;
// //     // cout << c.getA() << endl;
// //     // if (1 == 2) {

// //     // }

// //     // float a = 1;
// //     // unsigned long l;
// //     // memcpy(&l, &a, sizeof(float));
// //     // bitset<32> b(*reinterpret_cast<unsigned long*>(&a));
// //     // cout << (b  == 0b00111111100000000000000000000000) << endl;
// //     // cout << (l  == 0b00111111100000000000000000000000) << endl;
// //     // cout << l << endl;

// //     // vector<int> vec(10000000, 1);
// //     // int cur = vec[0];
// //     // for(size_t i = 1 ; i < vec.size() ; ++i) {
// //     //     cur += vec[i];
// //     //     vec[i] = cur;
// //     // }
// //     // cout << vec.back() << endl;
// //     // sort(vec.begin(), vec.end());
// //     // cin >> n >> d >> s;

// //     // long long ans = min(n, d * 2);
// //     // ans /= s;
// //     // ans *= s;
// //     // while(ans >= s * 2) {
// //     //     if (valid(ans)) {
// //     //         cout << ans << endl;
// //     //         return 0;
// //     //     }
// //     //     ans -= s;
// //     // }
// //     // cout << s << endl;
// //     return 0;
// // }
// // // 1000000000000
// // // 1000000000
// // // 9999999999

// // // 1000000000000
// // // 100000000000
// // // 10000000001

// // // 1000000000000
// // // 100000000000
// // // 10000000001


// // #include <atomic>
// // #include <iostream>

// // struct A
// // {
// //     virtual void asdf() {}
// //     char qmChar; // size: 1
// // public:
// //     virtual void qasdf() {}
// //     int mChar; // size: 1
// //     virtual void f(){}
// //     char amChar; // size: 1
// //     // double mInt; // size: 4
// // };

// // struct B 
// // {
// //     char mChar; // size: 1
// //     double mDouble; // size: 8
// //     int mInt; // size: 4
// // };

// // struct C
// // {
// //     double mDouble; // size: 8
// //     int mInt; // size: 4
// //     char mChar; // size: 1
// // };
 
// // int main()
// // {

// //     std::atomic<int> a(11);
// //     int d = 10;
// //     int c = 1;

// //     std::cout << a.compare_exchange_strong(d, c) << "\n";
// //     std::cout << a.load() << " " << d << " " << c << "\n";

// //     std::cout << "Size of struct A is: " << sizeof(A)
// //         << " with alignment: " << alignof(A) << std::endl;

// //     std::cout << "Size of struct B is: " << sizeof(B)
// //         << " with alignment: " << alignof(B) << std::endl;

// //     std::cout << "Size of struct C is: " << sizeof(C)
// //         << " with alignment: " << alignof(C) << std::endl;

// //     return 0;
// // }

// // List node
// // #include <atomic>
// // struct Node
// // {
// //     int mValue{0};
// //     Node* mNext{nullptr};
// // };

// // std::atomic<Node*> pHead(nullptr);

// // Push a node to the front of the list
// // void PushFront(const int& value)
// // {
// //     Node* newNode = new Node();
// //     newNode->mValue = value;
// //     Node* expected = pHead.load();
   
// //     // Try push the node we have just created to the front of the list
// //     do
// //     {
// //         newNode->mNext = pHead;
// //     }
// //     while(!pHead.compare_exchange_strong(expected, newNode));
// // }

// // #include <array>
// // #include <atomic>
// // #include <iostream>
// // #include <mutex>
// // #include <vector>
// // using namespace std;

// // template <class C>
// // struct B {
// //     void f() {
// //         return static_cast<C*>(this)->fImpl();
// //     }
// // };

// // struct S : public B<S> {
// //     S(int x):a(x){}
// //     int a;
// //     // virtual void fImpl() {}
// // };

// // struct A {
// //     char c[4];
// // };

// #include<bits/stdc++.h>
// #include <queue>
// #include <vector>
// using namespace std;
// struct cmp {
//     bool operator()(const int& a, const int& b) const {
//         return a < b;
//     }
//     void operator()(const int& a) const {
//         cout << a << endl;
//     }
// };

// bool gg(int a, int b) {
//     return a < b;
// }

// inline bool findEl(int x, const vector<int> &v) {
//     for(int i : v) {
//         if (i == x) return true;
//     }
//     return false;
// }

// // inline bool findEl(int x, vector<int> &v) {
// //     if (v.empty()) return false;
// //     if (v.back() == x) return true;
// //     v.pop_back();
// //     if (findEl(x, v)) return true;
// //     return false;
// // }
// inline int add(int a, int b) { return a + b; }


// int main() {

//     int (*ptr)(int, int) = add;   // address taken → compiler must emit standalone copy
//     ptr(2, 3);                     // call via pointer → cannot inline

//     vector<int> ve = {5, 1, 3};
//     cout << findEl(1, ve) << endl;
//     cmp a;
//     a(123213);
//     vector<int> v = {5, 1, 3};
//     sort(v.begin(), v.end(), gg);
//     for (int x : v) {
//         cout << x << " ";
//     }
//     cout << endl;
//     std::priority_queue<int, vector<int>, cmp> pq;
//     pq.push(5);
//     pq.push(1);
//     pq.push(3);
//     while (!pq.empty()) {
//         cout << pq.top() << " ";
//         pq.pop();
//     }
//     cout << endl;
//     // mutex mtx;
//     // mtx.lock();
//     // A a;
//     // cout << a.c << endl;
//     // mtx.unlock();
//     // std::atomic<S> s(4);
//     // s.store(0);
//     // std::cout << s.load().a << std::endl;
//     // std::atomic<std::array<int, 5>> ar;
//     // std::atomic<std::vector<int>> a;
//     return 0;
// }
