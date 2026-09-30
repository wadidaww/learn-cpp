// 06_error_category.cpp
//
// Lesson 6 — Custom Error Category
//
// The <system_error> framework provides a standard way to represent and
// communicate errors.  By defining a custom error category you can:
//   - Create domain-specific error codes (file, network, protocol)
//   - Map error codes to human-readable messages
//   - Throw typed exceptions (std::system_error)
//
// Key types:
//   std::error_category   — abstract base for error domains
//   std::error_code       — value + category pair
//   std::error_condition  — platform-independent error description
//   std::system_error     — exception wrapping error_code
//
// Patterns shown:
//   A. Defining error codes (enum class)
//   B. std::error_category subclass
//   C. std::error_code / std::error_condition
//   D. Error code to message mapping
//   E. std::system_error exception
//   F. Comparison with boost::system::error_code

#include <cerrno>
#include <cstring>
#include <exception>
#include <iostream>
#include <system_error>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char *title) {
  std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. Defining Error Codes
// ===========================================================================

enum class FileError {
  ok = 0,
  not_found,
  permission_denied,
  already_exists,
  disk_full,
  too_many_open,
  io_error
};

enum class NetworkError {
  ok = 0,
  connection_refused,
  connection_reset,
  host_unreachable,
  timeout,
  dns_failure,
  tls_error
};

namespace std {
template <> struct is_error_code_enum<FileError> : std::true_type {};
template <> struct is_error_code_enum<NetworkError> : std::true_type {};
} // namespace std

// ===========================================================================
// B. std::error_category Subclass
// ===========================================================================

class FileErrorCategory : public std::error_category {
public:
  static const FileErrorCategory &get() {
    static FileErrorCategory instance;
    return instance;
  }

  const char *name() const noexcept override { return "file_error"; }

  std::string message(int ev) const override {
    switch (static_cast<FileError>(ev)) {
    case FileError::ok:
      return "no error";
    case FileError::not_found:
      return "file not found";
    case FileError::permission_denied:
      return "permission denied";
    case FileError::already_exists:
      return "file already exists";
    case FileError::disk_full:
      return "disk full";
    case FileError::too_many_open:
      return "too many open files";
    case FileError::io_error:
      return "I/O error";
    default:
      return "unknown file error";
    }
  }

  std::error_condition default_error_condition(int ev) const noexcept override {
    switch (static_cast<FileError>(ev)) {
    case FileError::not_found:
      return std::errc::no_such_file_or_directory;
    case FileError::permission_denied:
      return std::errc::permission_denied;
    case FileError::disk_full:
      return std::errc::no_space_on_device;
    default:
      return {ev, *this};
    }
  }
};

class NetworkErrorCategory : public std::error_category {
public:
  static const NetworkErrorCategory &get() {
    static NetworkErrorCategory instance;
    return instance;
  }

  const char *name() const noexcept override { return "network_error"; }

  std::string message(int ev) const override {
    switch (static_cast<NetworkError>(ev)) {
    case NetworkError::ok:
      return "no error";
    case NetworkError::connection_refused:
      return "connection refused";
    case NetworkError::connection_reset:
      return "connection reset by peer";
    case NetworkError::host_unreachable:
      return "host unreachable";
    case NetworkError::timeout:
      return "connection timed out";
    case NetworkError::dns_failure:
      return "DNS resolution failed";
    case NetworkError::tls_error:
      return "TLS handshake failed";
    default:
      return "unknown network error";
    }
  }
};

std::error_code make_error_code(FileError e) {
  return {static_cast<int>(e), FileErrorCategory::get()};
}

std::error_code make_error_code(NetworkError e) {
  return {static_cast<int>(e), NetworkErrorCategory::get()};
}

// ===========================================================================
// C. std::error_code / std::error_condition
// ===========================================================================

void demo_error_code() {
  section("C. std::error_code / std::error_condition");

  std::error_code ec1 = FileError::not_found;
  std::error_code ec2 = NetworkError::timeout;

  std::cout << "  ec1: value=" << ec1.value()
            << " category=" << ec1.category().name() << " message=\""
            << ec1.message() << "\"\n";
  std::cout << "  ec2: value=" << ec2.value()
            << " category=" << ec2.category().name() << " message=\""
            << ec2.message() << "\"\n";

  std::cout << "  ec1 == FileError::not_found: "
            << (ec1 == FileError::not_found) << "\n";
  std::cout << "  ec1 == FileError::permission_denied: "
            << (ec1 == FileError::permission_denied) << "\n";

  std::cout << "  ec1 (bool): " << static_cast<bool>(ec1) << "\n";
  std::error_code no_error;
  std::cout << "  no_error (bool): " << static_cast<bool>(no_error) << "\n";

  std::error_condition cond = ec1.default_error_condition();
  std::cout << "  cond: category=" << cond.category().name() << " message=\""
            << cond.message() << "\"\n";
}

// ===========================================================================
// D. Error Code to Message Mapping
// ===========================================================================

void demo_message_mapping() {
  section("D. Error Code to Message Mapping");

  std::vector<std::error_code> errors = {FileError::ok,
                                         FileError::not_found,
                                         FileError::permission_denied,
                                         FileError::disk_full,
                                         NetworkError::connection_refused,
                                         NetworkError::timeout,
                                         NetworkError::dns_failure};

  for (const auto &ec : errors) {
    std::cout << "  [" << ec.category().name() << "] " << ec.value() << ": "
              << ec.message() << "\n";
  }
}

// ===========================================================================
// E. std::system_error Exception
// ===========================================================================

void throw_file_error(FileError e) {
  throw std::system_error(std::error_code(e), "failed to open config");
}

void throw_network_error(NetworkError e) {
  throw std::system_error(std::error_code(e), "connection failed");
}

void demo_system_error() {
  section("E. std::system_error Exception");

  try {
    throw_file_error(FileError::not_found);
  } catch (const std::system_error &ex) {
    std::cout << "  caught system_error:\n";
    std::cout << "    what(): " << ex.what() << "\n";
    std::cout << "    code(): " << ex.code().value() << " ("
              << ex.code().message() << ")\n";
    std::cout << "    category: " << ex.code().category().name() << "\n";
  }

  try {
    throw_network_error(NetworkError::timeout);
  } catch (const std::system_error &ex) {
    std::cout << "  caught system_error:\n";
    std::cout << "    what(): " << ex.what() << "\n";
    std::cout << "    code(): " << ex.code().value() << " ("
              << ex.code().message() << ")\n";
  }

  // Using with POSIX errors
  try {
    FILE *f = std::fopen("/nonexistent/path/file.txt", "r");
    if (!f) {
      throw std::system_error(std::error_code(errno, std::system_category()),
                              "open file");
    }
    std::fclose(f);
  } catch (const std::system_error &ex) {
    std::cout << "  POSIX error:\n";
    std::cout << "    what(): " << ex.what() << "\n";
    std::cout << "    code(): " << ex.code().message() << "\n";
  }
}

// ===========================================================================
// F. Comparison with boost::system::error_code
// ===========================================================================

void demo_boost_comparison() {
  section("F. Comparison: std::error_code vs boost::system::error_code");

  std::cout << "  std::error_code (C++11):\n";
  std::cout << "    #include <system_error>\n";
  std::cout << "    std::error_code ec = FileError::not_found;\n";
  std::cout << "    std::error_category is the base class\n";
  std::cout << "    std::system_error wraps it in an exception\n\n";

  std::cout << "  boost::system::error_code (Boost):\n";
  std::cout << "    #include <boost/system/error_code.hpp>\n";
  std::cout << "    boost::system::error_code ec(...);\n";
  std::cout << "    boost::system::error_category is the base class\n";
  std::cout << "    boost::system::system_error wraps it in an exception\n\n";

  std::cout << "  Key differences:\n";
  std::cout << "    - Both are value-semantic (copyable)\n";
  std::cout << "    - Both support custom categories via virtual dispatch\n";
  std::cout << "    - boost::system integrates with Boost.Asio (not in std)\n";
  std::cout << "    - In C++17+: std::error_code is sufficient for most uses\n";

  std::error_code std_ec = FileError::io_error;
  std::cout << "\n  std::error_code from FileError::io_error:\n";
  std::cout << "    message: " << std_ec.message() << "\n";
  std::cout << "    category: " << std_ec.category().name() << "\n";
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 6: Custom Error Category ===";

  demo_error_code();
  demo_message_mapping();
  demo_system_error();
  demo_boost_comparison();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
