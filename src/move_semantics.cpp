#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <utility>

class Buffer {
    size_t size_;
    int* data_;
public:
    explicit Buffer(size_t size)
        : size_(size), data_(new int[size]) {
        for (size_t i = 0; i < size_; ++i) data_[i] = 0;
        std::cout << "  [ctor] allocated " << size_ << " ints\n";
    }

    ~Buffer() {
        delete[] data_;
        std::cout << "  [dtor] freed " << size_ << " ints\n";
    }

    // Copy constructor — expensive
    Buffer(const Buffer& other)
        : size_(other.size_), data_(new int[other.size_]) {
        std::copy(other.data_, other.data_ + other.size_, data_);
        std::cout << "  [copy ctor] deep-copied " << size_ << " ints\n";
    }

    // Move constructor — cheap
    Buffer(Buffer&& other) noexcept
        : size_(other.size_), data_(other.data_) {
        other.size_ = 0;
        other.data_ = nullptr;
        std::cout << "  [move ctor] stolen " << size_ << " ints\n";
    }

    // Copy assignment
    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            delete[] data_;
            size_ = other.size_;
            data_ = new int[size_];
            std::copy(other.data_, other.data_ + size_, data_);
            std::cout << "  [copy assign] deep-copied " << size_ << " ints\n";
        }
        return *this;
    }

    // Move assignment
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;
            size_ = other.size_;
            data_ = other.data_;
            other.size_ = 0;
            other.data_ = nullptr;
            std::cout << "  [move assign] stolen " << size_ << " ints\n";
        }
        return *this;
    }

    [[nodiscard]] size_t size() const { return size_; }
};

Buffer createBuffer(size_t n) {
    Buffer b(n);
    return b;  // NRVO or move
}

void processByValue(Buffer b) {
    std::cout << "  processing buffer of size " << b.size() << "\n";
}

int main() {
    std::cout << "=== 1. Copy vs Move ===\n";
    {
        Buffer a(100);
        std::cout << "  -- copy a into b --\n";
        Buffer b(a);              // copy constructor
        std::cout << "  -- move a into c --\n";
        Buffer c(std::move(a));   // move constructor, a is now empty
    }

    std::cout << "\n=== 2. Return Value Optimization ===\n";
    {
        Buffer b = createBuffer(200);  // no copy, no move (NRVO)
    }

    std::cout << "\n=== 3. Move into vector ===\n";
    {
        std::vector<Buffer> vec;
        vec.reserve(3);
        std::cout << "  -- push_back with move --\n";
        vec.push_back(createBuffer(50));   // move (temporary is rvalue)
        std::cout << "  -- emplace_back --\n";
        vec.emplace_back(75);              // constructs in-place
    }

    std::cout << "\n=== 4. Perfect Forwarding ===\n";
    auto wrapper = [](auto&& arg) {
        processByValue(std::forward<decltype(arg)>(arg));
    };
    {
        Buffer b(30);
        std::cout << "  forwarding lvalue:\n";
        wrapper(b);                          // forwards as lvalue → copy
        std::cout << "  forwarding rvalue:\n";
        wrapper(Buffer(40));                 // forwards as rvalue → move
    }

    std::cout << "\n=== 5. std::move with strings ===\n";
    {
        std::string s = "Hello, C++17 move semantics!";
        std::string t = std::move(s);
        std::cout << "  s after move: \"" << s << "\" (size=" << s.size() << ")\n";
        std::cout << "  t after move: \"" << t << "\"\n";
    }

    std::cout << "\n=== 6. Move-only types (unique_ptr) ===\n";
    {
        auto p1 = std::make_unique<int>(42);
        // auto p2 = p1;              // ERROR: unique_ptr is not copyable
        auto p2 = std::move(p1);      // OK: move transfers ownership
        std::cout << "  *p2 = " << *p2 << "\n";
        std::cout << "  p1 after move: " << (p1 ? "valid" : "null") << "\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
