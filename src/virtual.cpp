#include <bits/stdc++.h>
using namespace std;

constexpr int getMaxIFact() {
    int i = 2, cur = 1;
    while(cur <= INT_MAX / (i + 1)) {
        cur *= i;
    }
    return i;
}

constexpr int getFact(const int K) {
    int res = 1;
    for(int i = 2 ; i <= K ; ++i) {
        res *= i;
    }
    return res;
}

constexpr int MAX_IFACT = getMaxIFact();
constexpr int MAX_FACT = getFact(MAX_IFACT);

inline constexpr int log2(int x) {
    int p2 = 1;
    int i = 0;
    for(i = 0 ; p2 <= x>>1 ; ++i, p2 <<= 1) {}
    return i;
}

static constexpr int N = 1e5;
static constexpr int L = log2(N);
static constexpr int P2 = 1<<L;


inline const int f() {
    return 42;
}

int av = 0;

const int x() {
    av = 10;
    return av;
}

void func() {
    static int x = 0;
    static int y = 0;
    x++;
    y += 2;
    cout << "x: " << x << ", y: " << y << endl;
}

int y;

const int *funPtr() {
    return &y;
}

class C {
    static const int x = 1;
    const int z;
    static const int y;
public:
    C():z(15) { }

    static const int get();

    const int *getXPtr() {
        return &x;
    }

    static const char *getChar() {
        return "Hello";
    }
};

struct MyStruct {
    int* data;
    size_t size;

    // Constructor
    MyStruct(size_t s) : size(s) {
        data = new int[size];
        // Initialize data
    }

    // Destructor
    ~MyStruct() {
        delete[] data;
    }

    // Copy Assignment Operator
    MyStruct& operator=(const MyStruct& other) {
        std::cout << "Copying other" << "\n";
        if (this != &other) { // Handle self-assignment
            delete[] data; // Free existing resources
            size = other.size;
            data = new int[size];
            for (size_t i = 0; i < size; ++i) {
                data[i] = other.data[i]; // Deep copy
            }
        }
        return *this;
    }

    // Move Assignment Operator
    MyStruct& operator=(MyStruct&& other) noexcept {
        std::cout << "Moving other" << "\n";
        if (this != &other) { // Handle self-assignment
            delete[] data; // Free existing resources
    
            data = other.data; // Transfer ownership
            size = other.size;
    
            other.data = nullptr; // Nullify source's resources
            other.size = 0;
        }
        return *this;
    }
};

// const int C::x = 12;
const int C::y = 10;

template <typename... Args>
void printArgs(Args&&... args) {
    // Using fold expression to print each argument
    std::cout << ">>>";
    (..., (std::cout << std::forward<Args>(args) << " "));
    std::cout << "\n";
    ((std::cout << std::forward<Args>(args) << " "), ...);
    std::cout << "\n";
    ((std::cout << 1 << " "), (std::cout << 2 << " "), (std::cout << 3 << " "));
    std::cout << "\n";

    std::cout << "<<<";
    std::cout << std::endl;
}

struct WithPadding {
    char a;      // 1 byte
    // 7 bytes padding
    double c;    // 8 bytes
    int b;       // 4 bytes
    // 4 bytes padding
    // 1 + 7 + 8 + 4 + 4 = 24
    // Total: 24 bytes (due to padding)
};

struct WithoutPadding {
    char a;      // 1 byte
    int b;       // 4 bytes
    // 3 padding bytes
    double c;    // 8 bytes
    // Total: 16 bytes (no padding)
    WithoutPadding() : a(0), b(0), c(0.0) {}
    WithoutPadding(int&& x) : a('a'), b(x), c(0.0) {
        free(&x);
    }
    WithoutPadding(long long &&x) = delete;
};

struct V {
    // V() {}
    // virtual void f() {
    //     std::cout << "V::f()" << std::endl;
    // }
};

struct IV : V {
    // IV() {}
    // void f() override {
    //     std::cout << "IV::f()" << std::endl;
    // }
};

class B {
    public:
    virtual void print() {
        cout << "B" <<"\n";
    }
};

class A : public B {
    public:
    void print() override {
        cout << "A" <<"\n";
    }
};

template <typename Z>
class X {
    public:
    void print() {
        cout << "B" <<"\n";
    }

    void call() {
        static_cast<Z*>(this)->callImpl();
    }
};

class Y : public X<Y> {
    public:
    void print()  {
        cout << "A" <<"\n";
    }

    void callImpl() {
        cout << "OH";
    }
};

class Q : public X<Q> {
    public:
    void print() {
        cout << "Q" << "\n";
    }
};

class TMP {
public:
    TMP(){
        cout <<" MAKE\n";
    }
    ~TMP() {
        cout << "DEL";
    }
};

void func(B *b) {
    b->print();
}

template <typename Z>
void func(Z *b) {
    cout << "dxfgxfgd";
    b->print();
}

void c(TMP c) {
    cout << "c" << &c << "\n";
    cout << "HcMM\n";
    // d(c);
}
void d(const TMP &&ca) {
    cout << "d" << &ca << "\n";
    cout << "HMddM\n";
    c(ca);
}

int main() {
    TMP l;
    // cout << " l " << &l << "  ---- \n";
    TMP *t = new TMP();
    // cout << " t " << &t << " ---- \n";
    t = new TMP();
    // cout << " " << &t << "---- \n";
    cout << " " << t << "---- \n";
    {

        c(*t);
        cout << " ---- \n";
    }
    cout << "@@@@@2" << &l << " " << "\n";
    {
        // int x = 1;
        // cout << x << "\n";
        d(std::move(l));
        // d(*t);
        cout << " ---- \n";
    }
    

    Y q;
    q.call();
    X<A> *b = new X<A>();
    Y *a = new Y();
    func(a);
    a->print();
    func(b);
    b->print();
    b = new X<A>();
    b->print();
    func(b);
    // B *b = new B();
    // A *a = new A();
    // func(a);
    // func(b);
    // func(b);
    // b = new A();
    // func(b);

    // int asdf = 1;
    // int const * ptr = &asdf;
    // // *ptr =10;
    // ptr = nullptr;

    // Allocates memory for an array of num objects of size and initializes all bytes in the allocated storage to zero.
        // IV aiv = IV();
        // V av = V();
        // cout << sizeof(av) << " " << sizeof(aiv) << "\n";
    // aiv.f();
    // av.f();

        // vector<long long> vll;
        // cout << sizeof(vll) << "\n";
        // vector<int> v;
        // v.reserve(3);
        // cout << v.size() << v.capacity() << "\n";
        // v.push_back(1);;
        // cout << v.size() << v.capacity() << "\n";
        // // v.reserve(10); 
        // cout << v.size() << v.capacity() << "\n";
        // v.push_back(1);;
        // cout << v.size() << v.capacity() << "\n";
        // v.push_back(1);;
        // cout << v.size() << v.capacity() << "\n";
        // v.push_back(1);;
        // cout << v.size() << v.capacity() << "\n";
        // v.push_back(1);;
        // cout << v.size() << v.capacity() << "\n";
        // cout << "sizeof(v)" << sizeof(v) << "\n";  
        // vector<int> aa;
        // cout << "sizeof(aa)" << sizeof(aa) << "\n";  
        // WithPadding p;
        // WithoutPadding wp;
        // cout << sizeof(p) << " " << sizeof(wp) << "\n";

        // int val = 10;
        // int *ptr = &val;
        // wp = WithoutPadding(std::move(*ptr));
        // cout << wp.a << " " << wp.b << " " << wp.c << "\n";
        // long long ll = 100;
    // wp = WithoutPadding(std::move(ll)); // This will cause a compile-time error
    // cout << val  << "\n";

        // printArgs(1, 2.5, "Hello", 'A'); // Example usage
        // printArgs(); // Example usage

        // MyStruct ms(10);
        // ms.data[5] = 10;
        // cout << ms.data[5] << "\n";

        // MyStruct ab(10);
        // ab = ms;
        // ab = std::move(ms);
        // int asdf = 1;

        // if (const bool x = true ; x) {
        //     cout << "x is true\n";
        // }


        // int x = 100;
        // int y = 1;
        // cout << &x << " " << &y << "\n";
        // cout << x << " " << y << "\n";
        // y = std::move(x);
        // cout << &x << " " << &y << "\n";
        // cout << x << " " << y << "\n";



    // vector<int> vec;
    // vec.reserve(10);
    // for(auto it = vec.begin() ; it != vec.end() ; ++it) {
        
    // }

    // C c = C();
    // const int *ptr = c.getXPtr();
    // void *tmp = &ptr;
    // cout << "ptr" << *ptr << " " << ptr << endl;
    // cout << C::getChar()<<"\n";

    // cout << L << " " << P2 << endl;

    // int *arr = static_cast<int *>(alloca(10 * sizeof(int)));
    // arr[5] = 11231;
    // cout << arr[5];

    // func();
    // func();
    // func();

    // unsigned i = 10;
    // cout << -1 + i;

    // int y = x();
    // y = 10;
    return 0;
}