// OPERATOR OVERLOADING
// =====================
// Key concepts:
//   1. Member vs free function: binary operators with LHS user type → member; LHS built-in → friend
//   2. Compound assignment (+=, -=, etc.): return *this for chaining
//   3. Subscript (operator[]): consider bounds checking, Proxy pattern for multi-dim
//   4. Function call (operator()): creates functors (callable objects, used with STL)
//   5. Conversion operators: explicit to prevent implicit conversions
//   6. Rule of thumb: operators should behave intuitively (don't redefine + to subtract)

#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <initializer_list>
#include <algorithm>

// 1. Arithmetic + comparison + stream operators on a 2D vector
class Vec2 {
    double x_, y_;
public:
    constexpr Vec2(double x = 0, double y = 0) : x_(x), y_(y) {}
    constexpr Vec2 operator+(const Vec2& r) const { return {x_+r.x_, y_+r.y_}; }
    constexpr Vec2 operator-(const Vec2& r) const { return {x_-r.x_, y_-r.y_}; }
    constexpr Vec2 operator*(double s) const { return {x_*s, y_*s}; }
    constexpr Vec2 operator-() const { return {-x_, -y_}; }
    Vec2& operator+=(const Vec2& r) { x_+=r.x_; y_+=r.y_; return *this; }
    constexpr bool operator==(const Vec2& r) const = default;
    constexpr double dot(const Vec2& r) const { return x_*r.x_ + y_*r.y_; }
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) { return os << "(" << v.x_ << "," << v.y_ << ")"; }
    friend constexpr Vec2 operator*(double s, const Vec2& v) { return v * s; } // LHS built-in
};

// 2. Subscript with bounds checking
class SafeArray {
    std::vector<int> data_;
public:
    explicit SafeArray(std::initializer_list<int> l) : data_(l) {}
    int& operator[](size_t i) { if (i>=data_.size()) throw std::out_of_range("out of range"); return data_[i]; }
};

// 3. Function call operator (functor) — objects you can "call"
class Multiplier {
    double factor_;
public:
    explicit Multiplier(double f) : factor_(f) {}
    double operator()(double x) const { return x * factor_; }
};

// 4. Increment/decrement (prefix returns ref, postfix returns old value)
class Counter {
    int v_;
public:
    explicit Counter(int v = 0) : v_(v) {}
    Counter& operator++() { ++v_; return *this; }
    Counter operator++(int) { auto tmp=*this; ++v_; return tmp; }
    friend std::ostream& operator<<(std::ostream& os, const Counter& c) { return os << c.v_; }
};

// 5. Explicit type conversion operators
class Temperature {
    double c_;
public:
    explicit Temperature(double c) : c_(c) {}
    explicit operator double() const { return c_; }
    explicit operator bool() const { return c_ > 0.0; }
};

// 6. Matrix with Proxy for m[i][j] syntax
class Matrix {
    size_t rows_, cols_;
    std::vector<double> data_;
    class Proxy {
        double* row_; size_t cols_;
    public:
        Proxy(double* r, size_t c) : row_(r), cols_(c) {}
        double& operator[](size_t j) { if (j>=cols_) throw std::out_of_range("col"); return row_[j]; }
    };
public:
    Matrix(size_t r, size_t c) : rows_(r), cols_(c), data_(r*c, 0.0) {}
    Proxy operator[](size_t i) { if (i>=rows_) throw std::out_of_range("row"); return Proxy(data_.data()+i*cols_, cols_); }
    friend std::ostream& operator<<(std::ostream& os, const Matrix& m) {
        for (size_t i=0;i<m.rows_;++i) { for (size_t j=0;j<m.cols_;++j) os<<m.data_[i*m.cols_+j]<<" "; os<<"\n"; } return os;
    }
};

int main() {
    // Arithmetic operators
    Vec2 a(1,2), b(3,4);
    std::cout << "a=" << a << " b=" << b << " a+b=" << a+b << " a*b=" << a*3 << " 3*a=" << 3.0*a << "\n";
    std::cout << "a==b:" << std::boolalpha << (a==b) << " a.dot(b)=" << a.dot(b) << "\n";

    // Subscript
    SafeArray arr({10,20,30});
    std::cout << "arr[1]=" << arr[1] << "\n";

    // Functor
    Multiplier times3(3.0);
    std::vector<double> v = {1,2,3,4,5};
    std::transform(v.begin(), v.end(), v.begin(), times3);
    std::cout << "times3: "; for (double x : v) std::cout << x << " "; std::cout << "\n";

    // Increment
    Counter c(5);
    std::cout << "c=" << c << " ++c=" << ++c << " c++=" << c++ << " c=" << c << "\n";

    // Type conversion
    Temperature t(36.6);
    std::cout << "temp as double: " << static_cast<double>(t) << " positive:" << (bool)t << "\n";

    // Matrix m[i][j]
    Matrix m(2,3);
    m[0][0]=1; m[0][1]=2; m[0][2]=3; m[1][0]=4; m[1][1]=5; m[1][2]=6;
    std::cout << "matrix:\n" << m;
}
