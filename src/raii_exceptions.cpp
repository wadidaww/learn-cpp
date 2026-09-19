// RAII & EXCEPTION SAFETY
// ========================
// Key concepts:
//   1. RAII (Resource Acquisition Is Initialization): tie resource lifetime to object scope
//   2. Destructor guarantees cleanup — even if exceptions are thrown
//   3. Exception safety levels: basic (invariants preserved), strong (commit-or-rollback), nothrow
//   4. noexcept: marks functions that won't throw (important for move constructors)
//   5. ScopeGuard: generic RAII helper for arbitrary cleanup actions

#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <numeric>

// --- RAII Wrapper: file handle automatically closed ---
class FileGuard {
    std::FILE* file_ = nullptr;
    std::string path_;
public:
    explicit FileGuard(const std::string& path) : path_(path) {
        file_ = std::fopen(path.c_str(), "w");
        if (!file_) throw std::runtime_error("Failed to open: " + path);
    }
    ~FileGuard() { if (file_) std::fclose(file_); }
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;
    void write(const std::string& data) { std::fputs(data.c_str(), file_); std::fputc('\n', file_); }
};

// --- Custom exceptions with context ---
class ValidationError : public std::runtime_error {
    std::string field_;
public:
    ValidationError(const std::string& field, const std::string& msg) : std::runtime_error(msg), field_(field) {}
    const std::string& field() const { return field_; }
};

// --- Exception-safe transfer (strong guarantee) ---
class BankAccount {
    std::string owner_; double balance_;
public:
    BankAccount(std::string owner, double balance) : owner_(std::move(owner)), balance_(balance) {}
    void transfer(BankAccount& to, double amount) {
        if (amount > balance_) throw std::logic_error("Insufficient funds");
        balance_ -= amount;
        try { to.balance_ += amount; }
        catch (...) { balance_ += amount; throw; }  // rollback on failure
    }
    friend std::ostream& operator<<(std::ostream& os, const BankAccount& a) { return os << a.owner_ << ": $" << a.balance_; }
};

// --- ScopeGuard: runs cleanup on scope exit ---
class ScopeGuard {
    std::function<void()> func_;
public:
    explicit ScopeGuard(std::function<void()> f) : func_(std::move(f)) {}
    ~ScopeGuard() { if (func_) func_(); }
    void release() { func_ = nullptr; }
};

// --- RAII via unique_ptr: zero-cost resource management ---
class Connection {
    std::string addr_;
public:
    explicit Connection(std::string addr) : addr_(std::move(addr)) { std::cout << "  Connected to " << addr_ << "\n"; }
    ~Connection() { std::cout << "  Disconnected from " << addr_ << "\n"; }
};

int main() {
    // 1. RAII file: automatically closed even if exception thrown
    std::cout << "=== RAII File ===\n";
    { FileGuard f("/tmp/raii_test.txt"); f.write("Hello RAII!"); }

    // 2. Exception handling: catch by reference, custom exceptions
    std::cout << "\n=== Custom Exceptions ===\n";
    try { throw ValidationError("email", "Invalid format"); }
    catch (const ValidationError& e) { std::cout << "  Field '" << e.field() << "': " << e.what() << "\n"; }

    // 3. Exception hierarchy: inner catch re-throws, outer catches by base type
    std::cout << "\n=== Exception Hierarchy ===\n";
    try {
        try { throw std::runtime_error("timeout"); }
        catch (const std::runtime_error& e) { std::cout << "  Inner: " << e.what() << "\n"; throw; }
    } catch (const std::exception& e) { std::cout << "  Outer: " << e.what() << "\n"; }

    // 4. Strong exception safety: state unchanged if transfer fails
    std::cout << "\n=== Exception-Safe Transfer ===\n";
    BankAccount alice("Alice", 1000), bob("Bob", 500);
    try { alice.transfer(bob, 200); alice.transfer(bob, 2000); }
    catch (const std::exception& e) { std::cout << "  Failed: " << e.what() << " — " << alice << ", " << bob << "\n"; }

    // 5. ScopeGuard: deferred cleanup
    std::cout << "\n=== ScopeGuard ===\n";
    { int* data = new int[10]; ScopeGuard cleanup([&]{ delete[] data; std::cout << "  Freed array\n"; }); data[0] = 42; }

    // 6. unique_ptr as RAII: resource released when pointer goes out of scope
    std::cout << "\n=== unique_ptr RAII ===\n";
    { auto conn = std::make_unique<Connection>("192.168.1.1:8080"); }
}
