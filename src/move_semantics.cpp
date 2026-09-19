// MOVE SEMANTICS & RULE OF 5
// ===========================
// Key concepts:
//   1. Move constructor/assignment transfers resources (steal) instead of copying
//   2. std::move casts to rvalue reference, enabling move overloads
//   3. NRVO (Named Return Value Optimization) eliminates copies on return
//   4. Rule of 5: if you define any of {dtor, copy ctor, copy assign, move ctor, move assign}, define all five
//   5. emplace_back constructs in-place, avoiding move entirely

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <utility>

class Buffer {
    size_t size_;
    int* data_;
public:
    explicit Buffer(size_t size) : size_(size), data_(new int[size]{}) {
        std::cout << "  [ctor] " << size_ << " ints\n";
    }
    ~Buffer() { delete[] data_; }

    // Rule of 5: copy and move operations
    Buffer(const Buffer& o) : size_(o.size_), data_(new int[o.size_]) {
        std::copy(o.data_, o.data_ + o.size_, data_);
        std::cout << "  [copy] " << size_ << " ints\n";
    }
    Buffer(Buffer&& o) noexcept : size_(o.size_), data_(o.data_) { // MOVE: steal pointer
        o.size_ = 0; o.data_ = nullptr;
        std::cout << "  [move] " << size_ << " ints\n";
    }
    Buffer& operator=(const Buffer& o) {
        if (this != &o) { delete[] data_; size_ = o.size_; data_ = new int[size_]; std::copy(o.data_, o.data_ + size_, data_); }
        return *this;
    }
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) { delete[] data_; size_ = o.size_; data_ = o.data_; o.size_ = 0; o.data_ = nullptr; }
        return *this;
    }
    [[nodiscard]] size_t size() const { return size_; }
};

Buffer createBuffer(size_t n) { return Buffer(n); } // NRVO: no copy/move
void processByValue(Buffer b) { std::cout << "  process size=" << b.size() << "\n"; }

int main() {
    // 1. Copy vs Move: copy duplicates data (expensive), move steals pointer (cheap)
    std::cout << "=== Copy vs Move ===\n";
    { Buffer a(100); Buffer b(a); Buffer c(std::move(a)); /* a is now empty */ }

    // 2. Return Value Optimization: compiler elides copy/move entirely
    std::cout << "\n=== NRVO ===\n";
    { Buffer b = createBuffer(200); }

    // 3. Move into container: push_back uses move for rvalues, emplace_back constructs in-place
    std::cout << "\n=== Move into vector ===\n";
    { std::vector<Buffer> vec; vec.reserve(2); vec.push_back(createBuffer(50)); vec.emplace_back(75); }

    // 4. Perfect Forwarding: preserve value category (lvalue stays lvalue, rvalue stays rvalue)
    std::cout << "\n=== Perfect Forwarding ===\n";
    auto wrapper = [](auto&& arg) { processByValue(std::forward<decltype(arg)>(arg)); };
    { Buffer b(30); wrapper(b); wrapper(Buffer(40)); }

    // 5. std::move with standard types: transfers internal buffer, leaves source in valid-but-empty state
    std::cout << "\n=== std::move string ===\n";
    { std::string s = "Hello"; std::string t = std::move(s); std::cout << "  s:\"" << s << "\" t:\"" << t << "\"\n"; }

    // 6. Move-only types: unique_ptr can't be copied, only moved (transfers ownership)
    std::cout << "\n=== unique_ptr (move-only) ===\n";
    { auto p1 = std::make_unique<int>(42); auto p2 = std::move(p1); std::cout << "  *p2=" << *p2 << " p1=" << (p1 ? "valid" : "null") << "\n"; }
}
