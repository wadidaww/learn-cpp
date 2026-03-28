
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

template<typename T>
class SmartPtr {
    int *cnt;
    T *ptr;
public:
    SmartPtr():cnt(new int(0)), ptr(nullptr) {}
    SmartPtr(T *obj):cnt(new int(1)) {ptr = obj;}
    // SmartPtr(T *obj):cnt(new int(1)) {ptr = obj;}
    ~SmartPtr() {
        if (--*cnt == 0) {
            delete ptr;
            delete cnt;
        }
    }
};

class Q {
    friend ostream &operator<< (ostream &os, const Q &q) {
        os << q.x;
        return os;
    }
    int x;
public:
    operator int() const {
        return 11;
    }
    operator float() const {
        return 3.14;
    }
Q():x(0){}
};

// int main() {
//     int *p = new int(10);
//     SmartPtr<int> sp3(p);
//     Q q;
//     cout << q << "\n";
//     cout << static_cast<float>(q) << "\n";
//     cout << float(q) << "\n";
//     cout << (float)q << "\n";
//     // int x = 1;
//     // unique_ptr<int> up(make_unique<int>(x));
//     // shared_ptr<int> sp_(make_shared<int>(p));
//     // SmartPtr<int> sp(new int(10));
//     // SmartPtr<int> sp2(new int(p));
//     // A a;
//     // B c = A();
//     // c.f();
//     // B *asdf = new A();
//     // asdf->f();
//     // delete asdf;
    
//     // const int *y;
//     // C a = get(&y);
//     // C b = get2(&y);
//     // cout << a.x << " " << b.x << endl;
//     // int q = 1;
//     // y = &q;
//     // cout << *y << endl;
//     // alignas(hardware_constructive_interference_size) int x = 10;
//     // fun(y);
//     // cout << *y << endl;
//     // double d = 10013.1;
//     // void *p;
//     // memcpy(&p, &d, sizeof(double));
//     // cout << p << endl;

//     // char *s;
//     // cin >> s;
//     // const char *__restrict ch = s;
//     // printf(s, 10);

//     // cout << myMax(10, 2, 5, 15.0, -1) << "\n";
//     // cout << std::max({10, 2, -5, 15, 1}) << "\n";
//     // int x = 10;
//     // class X {
//     //     int x;
//     // public:
//     //     ~X(){}
//     // };
//     // delete new X();
//     // X *asdf = new X();
//     // delete asdf;
//     // func(std::forward<int&>(x));
//     // func(10);
//     // func(int(1));
//     // func(std::forward<int&&>(20));
//     // int z = 1;
//     // int &&y = z + 10;
//     // y = x;
//     // func(y);
//     // cout << typeid(y).name() << endl;
//     // cout << y << endl;

//     // int a= 10;
//     // int &&b = a+10; // b is int &&
//     // auto c =b+10; // c is int
//     // auto &&d = a; // d is int&
//     // int &&di = a; // error, as expected
//     // const int ci = 10;       // Top-level const
//     // volatile double vd = 3.14; // Top-level volatile
//     // const volatile char cvc = 'a'; // Top-level const volatile

//     // const volatile int* p; // Low-level const (pointer to const int)
//     // func<>(p); // T = const volatile int* (cv-unqualified: low-level const remains)
 
//     // func<>(ci);  // T = int (cv-unqualified: const stripped)
//     // func(vd);  // T = double (volatile stripped)
//     // func(cvc); // T = char (const volatile stripped)
//     // func(12312); // T = char (const volatile stripped)
// }

// class Q {
//     friend ostream &operator<< (ostream &os, const Q &q) {
//         os << q.x;
//         return os;
//     }
//     int x;
// public:
//     operator int() const {
//         return 11;
//     }
//     operator float() const {
//         return 3.14;
//     }
// Q():x(0){}
// };

template<typename T>
void mod(T &x) {
    // x = 10;
}

int main() {
    const int x = 1;
    mod(x);
    int *a = new int(5);
    cout <<const_cast<const int*>(a);
    int *p = new int(10);
    SmartPtr<int> sp3(p);
    Q q;
    cout << q << "\n";
    cout << static_cast<float>(q) << "\n";
    // cout << dynamic_cast<float*>(q) << "\n";
    cout << float(q) << "\n";
    cout << (float)q << "\n";


    // int x = 1;
    // unique_ptr<int> up(make_unique<int>(x));
    // shared_ptr<int> sp_(make_shared<int>(p));
    // SmartPtr<int> sp(new int(10));
    // SmartPtr<int> sp2(new int(p));
    // A a;
    // B c = A();
    // c.f();
    // B *asdf = new A();
    // asdf->f();
    // delete asdf;
    
    // const int *y;
    // C a = get(&y);
    // C b = get2(&y);
    // cout << a.x << " " << b.x << endl;
    // int q = 1;
    // y = &q;
    // cout << *y << endl;
    // alignas(hardware_constructive_interference_size) int x = 10;
    // fun(y);
    // cout << *y << endl;
    // double d = 10013.1;
    // void *p;
    // memcpy(&p, &d, sizeof(double));
    // cout << p << endl;

    // char *s;
    // cin >> s;
    // const char *__restrict ch = s;
    // printf(s, 10);

    // cout << myMax(10, 2, 5, 15.0, -1) << "\n";
    // cout << std::max({10, 2, -5, 15, 1}) << "\n";
    // int x = 10;
    // class X {
    //     int x;
    // public:
    //     ~X(){}
    // };
    // delete new X();
    // X *asdf = new X();
    // delete asdf;
    // func(std::forward<int&>(x));
    // func(10);
    // func(int(1));
    // func(std::forward<int&&>(20));
    // int z = 1;
    // int &&y = z + 10;
    // y = x;
    // func(y);
    // cout << typeid(y).name() << endl;
    // cout << y << endl;

    // int a= 10;
    // int &&b = a+10; // b is int &&
    // auto c =b+10; // c is int
    // auto &&d = a; // d is int&
    // int &&di = a; // error, as expected
    // const int ci = 10;       // Top-level const
    // volatile double vd = 3.14; // Top-level volatile
    // const volatile char cvc = 'a'; // Top-level const volatile

    // const volatile int* p; // Low-level const (pointer to const int)
    // func<>(p); // T = const volatile int* (cv-unqualified: low-level const remains)
 
    // func<>(ci);  // T = int (cv-unqualified: const stripped)
    // func(vd);  // T = double (volatile stripped)
    // func(cvc); // T = char (const volatile stripped)
    // func(12312); // T = char (const volatile stripped)
}

/*

#include <iostream>
using namespace std;

// Class representing a reference counter class
class Counter 
{
public:
    // Constructor
    Counter() : m_counter(0){};

    Counter(const Counter&) = delete;
    Counter& operator=(const Counter&) = delete;

    // Destructor
    ~Counter() {}

    // count reset
    void reset()
    { 
      m_counter = 0;
    }

// getter
    unsigned int get() 
    { 
      return m_counter; 
    }

    // Overload post/pre increment
    void operator++() 
    { 
      m_counter++; 
    }

    void operator++(int) 
    { 
      m_counter++; 
    }

    // Overload post/pre decrement
    void operator--() 
    { 
      m_counter--; 
    }
    void operator--(int) 
    { 
      m_counter--; 
    }

    // Overloading << operator
    friend ostream& operator<<(ostream& os,
                               const Counter& counter)
    {
        os << "Counter Value : " << counter.m_counter
           << endl;
        return os;
    }

private:
    unsigned int m_counter{};
};


// Class template representing a shared pointer
template <typename T>
class Shared_ptr 
{
public:
    // Constructor
    Shared_ptr(T* ptr = nullptr)
    {
        m_ptr = ptr;
        m_counter = new unsigned(0);
        if (m_ptr) {
            (*m_counter)++;  
        }
    }


    // Copy constructor
    Shared_ptr(Shared_ptr<T>& sp)
    {
        // initializing shared pointer from other Shared_ptr object
        m_ptr = sp.m_ptr;
        
        // initializing reference counter from other shared pointer
        m_counter = sp.m_counter;
        (*m_counter)++;
    }

    // reference count getter
    unsigned int use_count() 
    { 
      return *m_counter; 
    }

    // shared pointer getter
    T* get() 
    { 
      return m_ptr;
    }

    // Overload * operator
    T& operator*() 
    { 
      return *m_ptr; 
    }

    // Overload -> operator
    T* operator->() 
    { 
      return m_ptr;
    }
    
    // overloading the = operator
    void operator= (Shared_ptr sp) {
        // if assigned pointer points to the some other location
        if (m_ptr != sp.m_ptr) {
            // if shared pointer already points to some location
            if(m_ptr && m_counter) {
                // decrease the reference counter for the previously pointed location
                (*m_counter)--;
                // if reference count reaches 0, deallocate memory
                if((*m_counter) == 0) {
                    delete m_counter;
                    delete m_ptr;
                }
            }
            // reference new memory location
            m_ptr = sp.m_ptr;
            // increase new memory location reference counter.
            if(m_ptr) {
                m_counter = sp.m_counter;
                (*m_counter)++;
            }
        }
    }
    
  
    // Destructor
    ~Shared_ptr()
    {
        (*m_counter)--;
        if (*m_counter == 0) 
        {
            delete m_counter;
            delete m_ptr;
        }
    }

    friend ostream& operator<<(ostream& os,
                               Shared_ptr<T>& sp)
    {
        os << "Address pointed : " << sp.get() << endl;
        os << *(sp.m_counter) << endl;
        return os;
    }

private:
    // Reference counter
    unsigned* m_counter;

    // Shared pointer
    T* m_ptr;
};

int main()
{
    // ptr1 pointing to an integer.
    Shared_ptr<int> ptr1(new int(151));
    cout << "--- Shared pointers ptr1 ---\n";
    *ptr1 = 100;
    cout << " ptr1's value now: " << *ptr1 << endl;
    cout << ptr1;

    {
        // ptr2 pointing to same integer
        // which ptr1 is pointing to
        // Shared pointer reference counter
        // should have increased now to 2.
        Shared_ptr<int> ptr2 = ptr1;
        cout << "--- Shared pointers ptr1, ptr2 ---\n";
        cout << ptr1;
        cout << ptr2;

        {
            // ptr3 pointing to same integer
            // which ptr1 and ptr2 are pointing to.
            // Shared pointer reference counter
            // should have increased now to 3.
            Shared_ptr<int> ptr3(ptr2);
            cout << "--- Shared pointers ptr1, ptr2, ptr3 "
                    "---\n";
            cout << ptr1;
            cout << ptr2;
            cout << ptr3;
        }
        
        Shared_ptr<int> temp(new int(11));
        ptr2 = temp;

        // ptr3 is out of scope.
        // It would have been destructed.
        // So shared pointer reference counter
        // should have decreased now to 2.
        cout << "--- Shared pointers ptr1, ptr2 ---\n";
        cout << ptr1;
        cout << ptr2;
    }

    // ptr2 is out of scope.
    // It would have been destructed.
    // So shared pointer reference counter
    // should have decreased now to 1.
    cout << "--- Shared pointers ptr1 ---\n";
    cout << ptr1;

    return 0;
}

*/