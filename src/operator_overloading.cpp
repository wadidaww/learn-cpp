#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <initializer_list>
#include <algorithm>
#include <typeinfo>

// --- 1. Arithmetic operators ---
class Vec2 {
    double x_, y_;
public:
    constexpr Vec2(double x = 0, double y = 0) : x_(x), y_(y) {}

    // Arithmetic
    constexpr Vec2 operator+(const Vec2& rhs) const { return {x_ + rhs.x_, y_ + rhs.y_}; }
    constexpr Vec2 operator-(const Vec2& rhs) const { return {x_ - rhs.x_, y_ - rhs.y_}; }
    constexpr Vec2 operator*(double scalar) const { return {x_ * scalar, y_ * scalar}; }
    constexpr Vec2 operator/(double scalar) const { return {x_ / scalar, y_ / scalar}; }
    constexpr Vec2 operator-() const { return {-x_, -y_}; }

    // Compound assignment
    Vec2& operator+=(const Vec2& rhs) { x_ += rhs.x_; y_ += rhs.y_; return *this; }
    Vec2& operator-=(const Vec2& rhs) { x_ -= rhs.x_; y_ -= rhs.y_; return *this; }
    Vec2& operator*=(double s) { x_ *= s; y_ *= s; return *this; }

    // Comparison
    constexpr bool operator==(const Vec2& rhs) const = default;
    constexpr bool operator!=(const Vec2& rhs) const = default;

    // Dot product
    constexpr double dot(const Vec2& rhs) const { return x_ * rhs.x_ + y_ * rhs.y_; }
    constexpr double length() const { return std::sqrt(dot(*this)); }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x_ << ", " << v.y_ << ")";
    }

    // Scalar * Vec2 (free function)
    friend constexpr Vec2 operator*(double s, const Vec2& v) { return v * s; }
};

// --- 2. Subscript operator (with bounds checking) ---
class SafeArray {
    std::vector<int> data_;
public:
    explicit SafeArray(std::initializer_list<int> list) : data_(list) {}

    int& operator[](size_t i) {
        if (i >= data_.size()) throw std::out_of_range("Index " + std::to_string(i) + " out of range");
        return data_[i];
    }

    const int& operator[](size_t i) const {
        if (i >= data_.size()) throw std::out_of_range("Index " + std::to_string(i) + " out of range");
        return data_[i];
    }

    size_t size() const { return data_.size(); }
};

// --- 3. Function call operator (functor) ---
class Multiplier {
    double factor_;
public:
    explicit Multiplier(double f) : factor_(f) {}
    double operator()(double x) const { return x * factor_; }
    double operator()(double x, double y) const { return x * y * factor_; }
};

// --- 4. Increment / Decrement ---
class Counter {
    int value_;
public:
    explicit Counter(int v = 0) : value_(v) {}

    Counter& operator++() { ++value_; return *this; }     // prefix
    Counter operator++(int) { auto tmp = *this; ++value_; return tmp; } // postfix
    Counter& operator--() { --value_; return *this; }
    Counter operator--(int) { auto tmp = *this; --value_; return tmp; }

    friend std::ostream& operator<<(std::ostream& os, const Counter& c) {
        return os << "Counter(" << c.value_ << ")";
    }
};

// --- 5. Dereference and member access ---
class PointerWrapper {
    int* ptr_;
public:
    explicit PointerWrapper(int* p) : ptr_(p) {}
    ~PointerWrapper() { delete ptr_; }

    int& operator*() { return *ptr_; }
    const int& operator*() const { return *ptr_; }
    int* operator->() { return ptr_; }
    const int* operator->() const { return ptr_; }

    PointerWrapper(const PointerWrapper&) = delete;
    PointerWrapper& operator=(const PointerWrapper&) = delete;
};

struct Person {
    std::string name;
    int age;
};

// --- 6. Type conversion operators ---
class Temperature {
    double celsius_;
public:
    explicit Temperature(double c) : celsius_(c) {}

    explicit operator double() const { return celsius_; }
    explicit operator bool() const { return celsius_ > 0.0; }

    friend std::ostream& operator<<(std::ostream& os, const Temperature& t) {
        return os << t.celsius_ << "°C";
    }
};

// --- 7. Comma operator ---
class Vector3 {
    double x_, y_, z_;
public:
    constexpr Vector3(double x, double y, double z) : x_(x), y_(y), z_(z) {}
    constexpr Vector3 operator,(const Vector3& rhs) const {
        return rhs;  // comma returns right operand
    }
    constexpr double x() const { return x_; }
    constexpr double y() const { return y_; }
    constexpr double z() const { return z_; }
    friend std::ostream& operator<<(std::ostream& os, const Vector3& v) {
        return os << "(" << v.x_ << ", " << v.y_ << ", " << v.z_ << ")";
    }
};

// --- 8. new / delete operators ---
class TrackedAllocator {
    static int count_;
public:
    void* operator new(size_t size) {
        ++count_;
        std::cout << "  [new] allocating " << size << " bytes (total: " << count_ << ")\n";
        return ::operator new(size);
    }

    void operator delete(void* ptr, size_t size) {
        --count_;
        std::cout << "  [delete] freeing " << size << " bytes (total: " << count_ << ")\n";
        ::operator delete(ptr);
    }

    static int count() { return count_; }
};
int TrackedAllocator::count_ = 0;

// --- 9. Subscript operator with two dimensions ---
class Matrix {
    size_t rows_, cols_;
    std::vector<double> data_;
public:
    Matrix(size_t r, size_t c) : rows_(r), cols_(c), data_(r * c, 0.0) {}

    class Proxy {
        double* row_ptr_;
        size_t cols_;
    public:
        Proxy(double* row_ptr, size_t cols) : row_ptr_(row_ptr), cols_(cols) {}
        double& operator[](size_t j) {
            if (j >= cols_) throw std::out_of_range("Column out of range");
            return row_ptr_[j];
        }
    };

    Proxy operator[](size_t i) {
        if (i >= rows_) throw std::out_of_range("Row out of range");
        return Proxy(data_.data() + i * cols_, cols_);
    }

    friend std::ostream& operator<<(std::ostream& os, const Matrix& m) {
        for (size_t i = 0; i < m.rows_; ++i) {
            for (size_t j = 0; j < m.cols_; ++j)
                os << m.data_[i * m.cols_ + j] << " ";
            os << "\n";
        }
        return os;
    }
};

int main() {
    // --- 1. Arithmetic operators ---
    std::cout << "=== 1. Arithmetic Operators ===\n";
    Vec2 a(1, 2), b(3, 4);
    std::cout << "  a = " << a << "\n";
    std::cout << "  b = " << b << "\n";
    std::cout << "  a + b = " << a + b << "\n";
    std::cout << "  a - b = " << a - b << "\n";
    std::cout << "  a * 3 = " << a * 3 << "\n";
    std::cout << "  3 * a = " << 3.0 * a << "\n";
    std::cout << "  -a = " << -a << "\n";
    std::cout << "  a == b: " << std::boolalpha << (a == b) << "\n";
    std::cout << "  a.dot(b) = " << a.dot(b) << "\n";
    std::cout << "  a.length() = " << a.length() << "\n";

    a += b;
    std::cout << "  a += b: " << a << "\n";

    // --- 2. Subscript ---
    std::cout << "\n=== 2. Subscript Operator ===\n";
    SafeArray arr({10, 20, 30, 40, 50});
    std::cout << "  arr[2] = " << arr[2] << "\n";
    try {
        arr[10];  // throws
    } catch (const std::out_of_range& e) {
        std::cout << "  arr[10]: " << e.what() << "\n";
    }

    // --- 3. Function call operator ---
    std::cout << "\n=== 3. Function Call Operator (Functor) ===\n";
    Multiplier times3(3.0);
    std::cout << "  times3(5) = " << times3(5.0) << "\n";
    std::cout << "  times3(2, 3) = " << times3(2.0, 3.0) << "\n";

    // Use with STL
    std::vector<double> vals = {1, 2, 3, 4, 5};
    std::transform(vals.begin(), vals.end(), vals.begin(), Multiplier(10.0));
    std::cout << "  transformed: ";
    for (double v : vals) std::cout << v << " ";
    std::cout << "\n";

    // --- 4. Increment / Decrement ---
    std::cout << "\n=== 4. Increment / Decrement ===\n";
    Counter c(5);
    std::cout << "  " << c << "\n";
    std::cout << "  ++c = " << ++c << "\n";
    std::cout << "  c++ = " << c++ << " (returned old)\n";
    std::cout << "  c = " << c << "\n";
    std::cout << "  --c = " << --c << "\n";
    std::cout << "  c-- = " << c-- << " (returned old)\n";
    std::cout << "  c = " << c << "\n";

    // --- 5. Dereference / member access ---
    std::cout << "\n=== 5. Dereference & Member Access ===\n";
    PointerWrapper pw(new int(42));
    std::cout << "  *pw = " << *pw << "\n";

    // --- 6. Type conversion ---
    std::cout << "\n=== 6. Type Conversion Operators ===\n";
    Temperature t(36.6);
    double val = static_cast<double>(t);
    std::cout << "  " << t << " as double: " << val << "\n";
    if (t) std::cout << "  temperature is positive\n";

    // --- 7. Comma operator ---
    std::cout << "\n=== 7. Comma Operator ===\n";
    Vector3 v1(1, 2, 3), v2(4, 5, 6);
    auto result = (v1, v2);
    std::cout << "  (v1, v2) = " << result << "\n";

    // --- 8. new / delete ---
    std::cout << "\n=== 8. Custom new/delete ===\n";
    std::cout << "  allocations: " << TrackedAllocator::count() << "\n";
    auto* p1 = new TrackedAllocator();
    auto* p2 = new TrackedAllocator();
    std::cout << "  allocations: " << TrackedAllocator::count() << "\n";
    delete p1;
    delete p2;
    std::cout << "  allocations: " << TrackedAllocator::count() << "\n";

    // --- 9. Matrix subscript ---
    std::cout << "\n=== 9. Matrix double subscript ===\n";
    Matrix m(3, 3);
    m[0][0] = 1; m[0][1] = 2; m[0][2] = 3;
    m[1][0] = 4; m[1][1] = 5; m[1][2] = 6;
    m[2][0] = 7; m[2][1] = 8; m[2][2] = 9;
    std::cout << m;

    std::cout << "\nDone.\n";
    return 0;
}
