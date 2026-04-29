#include <bits/stdc++.h>
#include <memory>
#include <ostream>
using namespace std;

void fun(shared_ptr<int> &ptr) {
    ptr = make_shared<int>(*new int(10));
}

void fun2(shared_ptr<int> &ptr) {
    int *x = new int(11);
    ptr.reset(x); // reference count decreases to 0, ptr now owns x, but x is not deleted because it is not managed by a shared_ptr anymore
}

class C {
    int *x;
public:
    C(int x): x(new int(x)) {
        cout << "Constructor called for C with x = " << *this->x << endl;
    }
    ~C() {
        cout << "Destructor called for C with x = " << *x << endl;
        delete x;
    }
    friend ostream& operator<<(ostream& os, const C& obj) {
        os << *obj.x;
        return os;
    }
};

void f(shared_ptr<C> &ptr) {
    ptr = make_shared<C>(10);
}

void fx(shared_ptr<C> ptr) {
    int x = 1;
    ptr = make_shared<C>(x);
}

template<typename T>
void a(T&& t) {
    cout << &t << endl;
}

class X {
public:
    X(int &x): x(x) {
        cout << "lvalue reference constructor called with x = " << x << endl;
    }
    X(int &&x) : x(x) {cout << "rvalue reference constructor called with x = " << x << endl;}
    friend ostream &operator<<(ostream& os, const X& obj) {
        os << "X object";
        return os;
    }
    int x;
};

// consteval int f(int x) {
//     return x * x;
// }

union A {
    int a;
    float b;
};


int main(int argc, char *argv[]) {
    string s;
    cout << sizeof(s) << "\n";
    A c;
    c.a = 5;
    // c.b = 1;
    cout << c.a << endl; // Output: 1 (the value of a is overwritten by the value of b)
    cout << c.b << endl; // Output: 1
    // atomic<S> s;
    // s.store(S{3, 4});
    X x(10);
    cout << &x << endl;
    a(x); // 1 move + 1 copy
    a(std::move(x)); // 2 move
    // C *a = new C(5);
    shared_ptr<C> p1; f(p1);
    shared_ptr<C> p2(p1);

    atomic<int> ref_count(1);

    
    // shared_ptr<C> p2(p1);
    // what is the differences between p1 and p2?
    // p1 and p2 are both shared_ptrs that manage the same raw pointer a 
    // the differences are:
    // p1 is initialized with the address of the object pointed to by a, which is the same as a, but p1 does not take ownership of the memory allocated for a, so when p1 goes out of scope, it will not delete the memory allocated for a, and the reference count of p1 will be 0, but the memory allocated for a will not be deleted
    // p2 is initialized with the raw pointer a, which means p2 takes ownership of the memory allocated for a, so when p2 goes out of scope, it will delete the memory allocated for a, and the reference count of p2 will be 1, but the memory allocated for a will be deleted when p2 goes out of scope, which is a bug because p1 also manages the same memory allocated for a, and when p1 goes out of scope, it will try to delete the memory allocated for a again, which will cause undefined behavior (double free error)

    // reference count is 1
    // reference count is 1 (p1 and p2 are independent shared_ptrs managing the same raw pointer, which is a bug)
    
    // cout << &*a << endl; // Output: address of a
    cout << *p1 << endl; // Output: 5
    cout << *p2 << endl; // Output: 5
    cout << p1.use_count() << endl; // Output: 2
    cout << p2.use_count() << endl; // Output: 2

    // consteval auto f = [](int x) {
    //     return x * x;
    // };

    // constexpr int z = f(5); // evaluated at compile time
    // int y = f(8); // evaluated at compile time, but not a constant expression, so it can be used in runtime contexts
    
    // shared_ptr<int> ptr(new int(5));
    // shared_ptr<int> ptr2 = ptr; // reference count increases to 2
    // shared_ptr<int> ptr3(ptr); // reference count increases to 3
    // fun(ptr);
    // fun2(ptr);
    // // shared_ptr<int> ptr4(std::move(ptr)); // ptr is now empty, reference count remains 3
    // cout << (ptr == nullptr) << endl;
    // cout << *ptr << endl;
    // cout << *ptr2 << endl; // Output: 5
    // cout << *ptr3 << endl; // Output: 5
    // // cout << *ptr4 << endl; // Output: 5
    // cout << ptr.use_count() << endl; // Output: 0 (ptr is empty)
    // cout << ptr2.use_count() << endl; // Output: 3 (ptr2, ptr3, and ptr4 share ownership)
    // cout << ptr3.use_count() << endl; // Output: 3 (ptr2, ptr3, and ptr4 share ownership)
    // // cout << ptr4.use_count() << endl; // Output: 3 (ptr2, ptr3, and ptr4 share ownership)
    // ptr.reset(); // reference count decreases to 1
    // cout << (ptr ? "ptr is not null" : "ptr is null") << endl; // Output: ptr is null
    // cout << (ptr2 ? "ptr2 is not null" : "ptr2 is null") << endl; // Output: ptr2 is not null


    return 0;
}