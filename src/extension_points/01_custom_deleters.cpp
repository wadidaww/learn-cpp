// 01_custom_deleters.cpp
//
// Lesson 1 — Custom Deleters for Smart Pointers
//
// Smart pointers take a Deleter template parameter that controls how the
// managed resource is released.  The default is std::default_delete<T>,
// which calls `delete p`.  By providing a custom deleter you can:
//
//   - Manage C resources (FILE*, socket, GPU memory, handles)
//   - Use lambdas, function pointers, or stateful function objects
//   - Stack-allocate small buffers and only heap-allocate when needed
//
// Patterns shown:
//   A. FILE* deleter — RAII wrapper for C file handles
//   B. Socket deleter — RAII wrapper for network sockets
//   C. Lambda deleter — stateless and stateful lambdas
//   D. shared_ptr aliasing constructor
//   E. unique_ptr with array deleter
//   F. unique_ptr vs shared_ptr — ownership semantics comparison

#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// POSIX sockets (for the socket deleter example)
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char *title) {
  std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. FILE* Deleter
// ===========================================================================
// C file handles must be closed with fclose(), not delete.
// A custom deleter wraps fclose() into a callable that unique_ptr invokes.

struct FileDeleter {
  void operator()(FILE *fp) const {
    if (fp) {
      std::cout << "  [FileDeleter] closing FILE*\n";
      std::fclose(fp);
    }
  }
};

using UniqueFile = std::unique_ptr<FILE, FileDeleter>;

// A generic RAII wrapper that also tracks open/close
class FileGuard {
public:
  explicit FileGuard(const char *path, const char *mode) {
    fp_ = std::fopen(path, mode);
    if (fp_)
      std::cout << "  [FileGuard] opened " << path << "\n";
    else
      std::cout << "  [FileGuard] failed to open " << path << "\n";
  }

  ~FileGuard() {
    if (fp_) {
      std::fclose(fp_);
      std::cout << "  [FileGuard] closed file\n";
    }
  }

  FileGuard(const FileGuard &) = delete;
  FileGuard &operator=(const FileGuard &) = delete;

  FileGuard(FileGuard &&o) noexcept : fp_(o.fp_) { o.fp_ = nullptr; }
  FileGuard &operator=(FileGuard &&o) noexcept {
    if (this != &o) {
      if (fp_)
        std::fclose(fp_);
      fp_ = o.fp_;
      o.fp_ = nullptr;
    }
    return *this;
  }

  FILE *get() const { return fp_; }
  bool valid() const { return fp_ != nullptr; }

private:
  FILE *fp_ = nullptr;
};

// ---------------------------------------------------------------------------
void demo_file_deleter() {
  section("A. FILE* Deleter — RAII for C file handles");

  // Using the custom deleter struct
  {
    UniqueFile f(std::fopen("/tmp/ext_test.txt", "w"));
    if (f) {
      std::fprintf(f.get(), "Hello from custom deleter!\n");
      std::cout << "  wrote to file via UniqueFile\n";
    }
    // f goes out of scope -> FileDeleter::operator() called -> fclose()
  }
  std::cout << "  after scope: file closed automatically\n";

  // Using the RAII wrapper class
  {
    FileGuard guard("/tmp/ext_test2.txt", "w");
    if (guard.valid()) {
      std::fprintf(guard.get(), "Hello from FileGuard!\n");
      std::cout << "  wrote to file via FileGuard\n";
    }
  }
  std::cout << "  after scope: FileGuard destructor called\n";
}

// ===========================================================================
// B. Socket Deleter
// ===========================================================================
// Sockets created with socket() must be closed with close().
// A struct deleter is the cleanest pattern for socket resources.

struct SocketDeleter {
  void operator()(int *fd) const {
    if (fd && *fd >= 0) {
      std::cout << "  [SocketDeleter] closing fd " << *fd << "\n";
      ::close(*fd);
    }
    delete fd;
  }
};

using UniqueSocket = std::unique_ptr<int, SocketDeleter>;

void demo_socket_deleter() {
  section("B. Socket Deleter — RAII for network sockets");

  // Create a socket and transfer ownership
  {
    int raw_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    std::cout << "  created socket fd=" << raw_fd << "\n";

    UniqueSocket sock(new int(raw_fd), SocketDeleter{});
    std::cout << "  socket owned by unique_ptr, fd=" << *sock << "\n";
    // Destructor calls close() via the deleter
  }

  // Lambda deleter alternative
  {
    int fd2 = ::socket(AF_INET, SOCK_STREAM, 0);
    auto deleter = [](int *p) {
      if (p && *p >= 0) {
        std::cout << "  [lambda deleter] closing fd " << *p << "\n";
        ::close(*p);
      }
      delete p;
    };
    std::unique_ptr<int, decltype(deleter)> sock(new int(fd2), deleter);
    std::cout << "  lambda-managed socket fd=" << *sock << "\n";
  }
}

// ===========================================================================
// C. Lambda Deleters — Stateless and Stateful
// ===========================================================================
// Lambdas as deleters can capture state. This is useful for:
//   - Logging deallocations
//   - Returning memory to a pool
//   - Tracking resource counts

void demo_lambda_deleters() {
  section("C. Lambda Deleters — stateless and stateful");

  // Stateless lambda (same as a function pointer, but inlined)
  {
    auto deleter = [](int *p) {
      std::cout << "  [stateless lambda] deleting int=" << *p << "\n";
      delete p;
    };
    std::unique_ptr<int, decltype(deleter)> p(new int(42), deleter);
    std::cout << "  value=" << *p << "\n";
  }
  std::cout << "  after scope: stateless lambda deleter called\n";

  // Stateful lambda — tracks total bytes freed
  struct Stats {
    size_t bytes_freed = 0;
  };
  Stats stats;

  auto tracked_deleter = [&stats](char *p) {
    std::cout << "  [tracked deleter] freeing pointer, total freed="
              << stats.bytes_freed << " bytes\n";
    delete[] p;
  };

  {
    std::unique_ptr<char[], decltype(tracked_deleter)> buf(new char[1024],
                                                           tracked_deleter);
    std::cout << "  allocated 1024-byte buffer\n";
  }
  std::cout << "  after scope: tracked lambda deleter called\n";

  // Deleter as a stateful functor
  struct PoolReturner {
    std::string pool_name;

    void operator()(int *p) const {
      std::cout << "  [PoolReturner] returning int=" << *p << " to pool '"
                << pool_name << "'\n";
      delete p;
    }
  };

  {
    std::unique_ptr<int, PoolReturner> p(new int(99), {"main_pool"});
    std::cout << "  value=" << *p << "\n";
  }
}

// ===========================================================================
// D. shared_ptr Aliasing Constructor
// ===========================================================================
// The aliasing constructor creates a shared_ptr that shares ownership with
// another shared_ptr but points to a different object (or sub-object).

struct Sensor {
  int id;
  std::string name;
  double calibration_offset;

  Sensor(int i, std::string n, double c)
      : id(i), name(std::move(n)), calibration_offset(c) {
    std::cout << "  [Sensor] created Sensor#" << id << "\n";
  }
  ~Sensor() { std::cout << "  [Sensor] destroyed Sensor#" << id << "\n"; }
};

void demo_aliasing_constructor() {
  section("D. shared_ptr Aliasing Constructor");

  // Create a shared_ptr to a Sensor
  auto sensor = std::make_shared<Sensor>(42, "temperature", 3.14);
  std::cout << "  sensor use_count=" << sensor.use_count() << "\n";

  // Create an aliasing shared_ptr to the name member
  std::shared_ptr<std::string> name_ptr(sensor, &sensor->name);
  std::cout << "  name_ptr use_count=" << name_ptr.use_count() << "\n";
  std::cout << "  *name_ptr=" << *name_ptr << "\n";

  std::cout << "  sensor use_count=" << sensor.use_count() << "\n";

  {
    std::shared_ptr<Sensor> s2 = sensor;
    std::shared_ptr<std::string> n2(sensor, &sensor->name);
    std::cout << "  sensor use_count=" << sensor.use_count() << "\n";
  }
  std::cout << "  after scope: sensor use_count=" << sensor.use_count() << "\n";

  // Practical use case
  struct DataStore {
    std::vector<std::shared_ptr<int>> data;

    DataStore() {
      for (int i = 0; i < 5; ++i)
        data.push_back(std::make_shared<int>(i * 10));
    }

    std::weak_ptr<int> get(int index) const {
      if (index >= 0 && index < static_cast<int>(data.size()))
        return data[index];
      return {};
    }
  };

  DataStore store;
  if (auto val = store.get(2).lock()) {
    std::cout << "  store.get(2) = " << *val << "\n";
  }
}

// ===========================================================================
// E. unique_ptr with Array Deleter
// ===========================================================================
// unique_ptr<T[]> uses delete[] by default, but you can customize it too.

void demo_array_deleter() {
  section("E. unique_ptr with Array Deleter");

  // Default array deleter
  {
    std::unique_ptr<int[]> arr(new int[5]{10, 20, 30, 40, 50});
    std::cout << "  default array: ";
    for (int i = 0; i < 5; ++i)
      std::cout << arr[i] << " ";
    std::cout << "\n";
  }
  std::cout << "  default array deleted\n";

  // Custom array deleter with logging
  auto logging_array_deleter = [](int *p) {
    std::cout << "  [logging_array_deleter] freeing array at " << p << "\n";
    delete[] p;
  };
  {
    std::unique_ptr<int[], decltype(logging_array_deleter)> arr(
        new int[3]{100, 200, 300}, logging_array_deleter);
    std::cout << "  logged array: ";
    for (int i = 0; i < 3; ++i)
      std::cout << arr[i] << " ";
    std::cout << "\n";
  }
  std::cout << "  logged array deleted\n";

  // shared_ptr with array deleter
  {
    std::shared_ptr<double> arr(new double[4]{1.1, 2.2, 3.3, 4.4},
                                [](double *p) {
                                  std::cout << "  [shared_ptr array deleter]\n";
                                  delete[] p;
                                });
    std::cout << "  shared array[0]=" << arr.get()[0] << "\n";
  }
}

// ===========================================================================
// F. unique_ptr vs shared_ptr — Ownership Semantics
// ===========================================================================

void demo_ownership_comparison() {
  section("F. unique_ptr vs shared_ptr — ownership semantics");

  // unique_ptr: exclusive ownership, zero overhead
  {
    auto uptr = std::make_unique<int>(100);
    std::cout << "  unique_ptr value=" << *uptr << "\n";
    std::cout << "  sizeof(unique_ptr<int>)=" << sizeof(uptr) << "\n";

    std::unique_ptr<int> moved = std::move(uptr);
    std::cout << "  after move: uptr=" << (uptr ? "valid" : "null") << "\n";
    std::cout << "  moved value=" << *moved << "\n";
  }

  // shared_ptr: shared ownership via reference counting
  {
    auto sptr = std::make_shared<int>(200);
    std::cout << "  shared_ptr value=" << *sptr
              << " use_count=" << sptr.use_count() << "\n";
    std::cout << "  sizeof(shared_ptr<int>)=" << sizeof(sptr) << "\n";

    {
      auto copy = sptr;
      std::cout << "  after copy: use_count=" << sptr.use_count() << "\n";
    }
    std::cout << "  after scope: use_count=" << sptr.use_count() << "\n";
  }

  std::cout << "\n  Performance characteristics:\n";
  std::cout << "  unique_ptr: sizeof = 1 ptr (usually 8 bytes)\n";
  std::cout << "  shared_ptr: sizeof = 2 ptrs (ptr + control block)\n";
  std::cout << "  unique_ptr: move = trivial (pointer swap)\n";
  std::cout << "  shared_ptr: copy = atomic increment on control block\n";

  std::cout << "\n  When to use which:\n";
  std::cout << "  unique_ptr: factory returns, pimpl, single ownership\n";
  std::cout << "  shared_ptr: shared caches, graphs, observer patterns\n";
  std::cout << "  weak_ptr:   break circular references, cached references\n";
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 1: Custom Deleters for Smart Pointers ===";

  demo_file_deleter();
  demo_socket_deleter();
  demo_lambda_deleters();
  demo_aliasing_constructor();
  demo_array_deleter();
  demo_ownership_comparison();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
