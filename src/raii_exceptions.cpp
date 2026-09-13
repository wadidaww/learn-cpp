#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <functional>

// --- 1. RAII Basics ---
class FileGuard {
    std::string path_;
    std::FILE* file_ = nullptr;
public:
    explicit FileGuard(const std::string& path) : path_(path) {
        file_ = std::fopen(path.c_str(), "w");
        if (!file_) throw std::runtime_error("Failed to open: " + path);
        std::cout << "  FileGuard opened: " << path_ << "\n";
    }

    ~FileGuard() {
        if (file_) {
            std::fclose(file_);
            std::cout << "  FileGuard closed: " << path_ << "\n";
        }
    }

    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;

    void write(const std::string& data) {
        std::fputs(data.c_str(), file_);
        std::fputc('\n', file_);
    }
};

// --- 2. RAII with dynamically allocated memory ---
class IntArray {
    int* data_;
    size_t size_;
public:
    explicit IntArray(size_t n) : data_(new int[n]{}), size_(n) {
        std::cout << "  IntArray allocated " << n << " ints\n";
    }

    ~IntArray() {
        delete[] data_;
        std::cout << "  IntArray freed " << size_ << " ints\n";
    }

    IntArray(const IntArray&) = delete;
    IntArray& operator=(const IntArray&) = delete;

    int& operator[](size_t i) { return data_[i]; }
    const int& operator[](size_t i) const { return data_[i]; }
    size_t size() const { return size_; }
};

// --- 3. Custom exception types ---
class ValidationError : public std::runtime_error {
    std::string field_;
public:
    ValidationError(const std::string& field, const std::string& msg)
        : std::runtime_error(msg), field_(field) {}

    const std::string& field() const { return field_; }
};

class DatabaseError : public std::runtime_error {
    int code_;
public:
    DatabaseError(int code, const std::string& msg)
        : std::runtime_error(msg), code_(code) {}

    int error_code() const { return code_; }
};

// --- 4. Exception-safe code patterns ---
class BankAccount {
    std::string owner_;
    double balance_;
public:
    BankAccount(std::string owner, double balance)
        : owner_(std::move(owner)), balance_(balance) {}

    void transfer(BankAccount& to, double amount) {
        if (amount > balance_)
            throw std::logic_error("Insufficient funds");
        if (amount <= 0)
            throw std::invalid_argument("Amount must be positive");

        // Step-by-step with exception safety
        balance_ -= amount;           // if this throws, account unchanged
        try {
            to.balance_ += amount;    // if this throws, reverse the first
        } catch (...) {
            balance_ += amount;       // roll back
            throw;                    // re-throw
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const BankAccount& a) {
        return os << a.owner_ << ": $" << a.balance_;
    }
};

// --- 5. Exception hierarchies ---
struct AppException : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct NetworkException : AppException {
    using AppException::AppException;
};

struct TimeoutException : NetworkException {
    using NetworkException::NetworkException;
};

// --- 6. noexcept ---
void safe_move() noexcept {
    int x = 42;
    (void)x;
}

// --- 7. Scope Guard (RAII helper) ---
class ScopeGuard {
    std::function<void()> func_;
public:
    explicit ScopeGuard(std::function<void()> f) : func_(std::move(f)) {}
    ~ScopeGuard() { if (func_) func_(); }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    void release() { func_ = nullptr; }
};

// --- 8. Using std::unique_ptr for RAII ---
class Connection {
    std::string addr_;
public:
    explicit Connection(std::string addr) : addr_(std::move(addr)) {
        std::cout << "  Connection to " << addr_ << " established\n";
    }
    ~Connection() {
        std::cout << "  Connection to " << addr_ << " closed\n";
    }
    void send(const std::string& msg) {
        std::cout << "  [" << addr_ << "] " << msg << "\n";
    }
};

int main() {
    // --- 1. RAII file ---
    std::cout << "=== 1. RAII File ===\n";
    {
        FileGuard f("/tmp/raii_test.txt");
        f.write("Hello, RAII!");
        f.write("Line 2");
    } // file closed automatically

    // --- 2. RAII dynamic array ---
    std::cout << "\n=== 2. RAII Dynamic Array ===\n";
    {
        IntArray arr(5);
        for (size_t i = 0; i < arr.size(); ++i) arr[i] = static_cast<int>(i * i);
        for (size_t i = 0; i < arr.size(); ++i) std::cout << arr[i] << " ";
        std::cout << "\n";
    } // memory freed automatically

    // --- 3. Exception handling ---
    std::cout << "\n=== 3. Exception Handling ===\n";
    try {
        throw ValidationError("email", "Invalid email format");
    } catch (const ValidationError& e) {
        std::cout << "  Caught ValidationError on field '" << e.field()
                  << "': " << e.what() << "\n";
    }

    try {
        throw DatabaseError(404, "Record not found");
    } catch (const DatabaseError& e) {
        std::cout << "  Caught DatabaseError [" << e.error_code()
                  << "]: " << e.what() << "\n";
    }

    // --- 4. Exception hierarchy ---
    std::cout << "\n=== 4. Exception Hierarchy ===\n";
    try {
        try {
            throw TimeoutException("Connection timed out");
        } catch (const TimeoutException& e) {
            std::cout << "  Inner catch (TimeoutException): " << e.what() << "\n";
            throw;
        }
    } catch (const NetworkException& e) {
        std::cout << "  Outer catch (NetworkException): " << e.what() << "\n";
    } catch (const AppException& e) {
        std::cout << "  AppException: " << e.what() << "\n";
    }

    // --- 5. Bank transfer (exception safety) ---
    std::cout << "\n=== 5. Exception-Safe Transfer ===\n";
    {
        BankAccount alice("Alice", 1000);
        BankAccount bob("Bob", 500);
        std::cout << "  Before: " << alice << ", " << bob << "\n";
        try {
            alice.transfer(bob, 200);
            std::cout << "  After:  " << alice << ", " << bob << "\n";
            alice.transfer(bob, 2000);  // throws
        } catch (const std::exception& e) {
            std::cout << "  Transfer failed: " << e.what() << "\n";
            std::cout << "  State preserved: " << alice << ", " << bob << "\n";
        }
    }

    // --- 6. Scope guard ---
    std::cout << "\n=== 6. Scope Guard ===\n";
    {
        int* data = new int[10];
        ScopeGuard cleanup([&]() {
            delete[] data;
            std::cout << "  ScopeGuard: freed array\n";
        });

        data[0] = 42;
        std::cout << "  data[0] = " << data[0] << "\n";
        // cleanup runs when scope ends
    }

    // --- 7. unique_ptr as RAII wrapper ---
    std::cout << "\n=== 7. unique_ptr RAII ===\n";
    {
        auto conn = std::make_unique<Connection>("192.168.1.1:8080");
        conn->send("Hello server!");
        // Connection closed automatically when unique_ptr goes out of scope
    }

    // --- 8. Exception in constructor ---
    std::cout << "\n=== 8. Constructor Exception ===\n";
    auto make_account = [](const std::string& name, double bal) -> std::unique_ptr<BankAccount> {
        if (bal < 0) throw std::invalid_argument("Balance cannot be negative");
        return std::make_unique<BankAccount>(name, bal);
    };
    try {
        auto acc = make_account("Bob", -100);
    } catch (const std::exception& e) {
        std::cout << "  Failed to create account: " << e.what() << "\n";
    }

    std::cout << "\nDone. All resources cleaned up.\n";
    return 0;
}
