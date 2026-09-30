// 05_streambuf_customization.cpp
//
// Lesson 5 — Custom streambuf
//
// std::basic_streambuf is the underlying I/O buffer that istream/ostream
// use.  By customizing it you can:
//   - Read from/write to custom sources (files, memory, network)
//   - Tee output to multiple destinations (console + file)
//   - Implement circular buffers for streaming data
//
// Key methods:
//   put area (output):  pbase(), pptr(), epptr(), overflow()
//   get area (input):   eback(), gptr(), egptr(), underflow()
//
// Patterns shown:
//   A. Streambuf anatomy overview
//   B. String streambuf (in-memory buffer)
//   C. Logging tee streambuf (console + file)
//   D. Memory-mapped file streambuf (read side)
//   E. Connecting custom streambuf to istream/ostream
//   F. Bidirectional streambuf (read + write)

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char *title) {
  std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. Streambuf Anatomy
// ===========================================================================

void demo_anatomy() {
  section("A. Streambuf Anatomy");

  std::cout << "  Put area (output):\n";
  std::cout << "    pbase()  = start of put buffer\n";
  std::cout << "    pptr()   = current write position\n";
  std::cout << "    epptr()  = end of put buffer\n";
  std::cout << "    overflow() called when pptr() == epptr()\n\n";

  std::cout << "  Get area (input):\n";
  std::cout << "    eback()  = start of get buffer\n";
  std::cout << "    gptr()   = current read position\n";
  std::cout << "    egptr()  = end of get buffer\n";
  std::cout << "    underflow() called when gptr() == egptr()\n\n";

  // Demonstrate with a stringbuf (public interface)
  std::stringbuf sbuf("Hello, streambuf!");
  std::cout << "  std::stringbuf demonstration:\n";
  std::cout << "    buffer: " << sbuf.str() << "\n";
  std::cout << "    first char (sgetc): " << static_cast<char>(sbuf.sgetc())
            << "\n";
  std::cout << "    second char (sbumpc): " << static_cast<char>(sbuf.sbumpc())
            << "\n";
  std::cout << "    after bumpc, sgetc: " << static_cast<char>(sbuf.sgetc())
            << "\n";
}

// ===========================================================================
// B. String Streambuf (In-Memory Buffer)
// ===========================================================================
// A simple growing buffer that captures all output.

class StringStreambuf : public std::streambuf {
public:
  StringStreambuf() = default;

  std::string str() const { return buffer_; }

protected:
  int_type overflow(int_type ch) override {
    if (traits_type::eof() != ch) {
      buffer_ += traits_type::to_char_type(ch);
    }
    return ch;
  }

private:
  std::string buffer_;
};

void demo_string_buffer() {
  section("B. String Streambuf — in-memory buffer");

  StringStreambuf sbuf;
  std::ostream os(&sbuf);

  os << "Hello, ";
  os << "Streambuf ";
  os << "World!";

  std::cout << "  captured: \"" << sbuf.str() << "\"\n";
  std::cout << "  length: " << sbuf.str().size() << " bytes\n";
}

// ===========================================================================
// C. Logging Tee Streambuf
// ===========================================================================

class TeeStreambuf : public std::streambuf {
public:
  TeeStreambuf(std::streambuf *original, std::ostream &file_stream)
      : original_(original), file_(file_stream) {}

protected:
  int_type overflow(int_type ch) override {
    if (traits_type::eof() != ch) {
      original_->sputc(ch);
      file_.put(traits_type::to_char_type(ch));
    }
    return ch;
  }

  int sync() override {
    original_->pubsync();
    file_.flush();
    return 0;
  }

private:
  std::streambuf *original_;
  std::ostream &file_;
};

void demo_tee_logging() {
  section("C. Logging Tee Streambuf — console + file");

  std::ofstream log_file("/tmp/ext_log.txt");
  if (!log_file) {
    std::cerr << "  failed to open log file\n";
    return;
  }

  TeeStreambuf tee(std::cout.rdbuf(), log_file);
  std::streambuf *old = std::cout.rdbuf(&tee);

  std::cout << "This goes to both console and file\n";
  std::cout << "Timestamp: 2026-01-01 12:00:00\n";
  std::cout << "Level: INFO\n";

  std::cout.rdbuf(old);
  log_file.close();

  std::ifstream verify("/tmp/ext_log.txt");
  std::string line;
  std::cout << "\n  Log file contents:\n";
  while (std::getline(verify, line)) {
    std::cout << "    " << line << "\n";
  }
}

// ===========================================================================
// D. Memory-Mapped File Streambuf (Read Side)
// ===========================================================================

class MmapStreambuf : public std::streambuf {
public:
  explicit MmapStreambuf(const char *filename) {
    fd_ = ::open(filename, O_RDONLY);
    if (fd_ < 0) {
      std::cerr << "  failed to open file: " << filename << "\n";
      return;
    }

    struct stat st;
    if (::fstat(fd_, &st) < 0) {
      ::close(fd_);
      fd_ = -1;
      return;
    }
    file_size_ = st.st_size;

    data_ = static_cast<char *>(
        ::mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0));
    if (data_ == MAP_FAILED) {
      data_ = nullptr;
      ::close(fd_);
      fd_ = -1;
      return;
    }

    setg(data_, data_, data_ + file_size_);
  }

  ~MmapStreambuf() {
    if (data_)
      ::munmap(data_, file_size_);
    if (fd_ >= 0)
      ::close(fd_);
  }

  MmapStreambuf(const MmapStreambuf &) = delete;
  MmapStreambuf &operator=(const MmapStreambuf &) = delete;

  std::size_t file_size() const { return file_size_; }

protected:
  int_type underflow() override {
    if (gptr() < egptr())
      return traits_type::to_int_type(*gptr());
    return traits_type::eof();
  }

private:
  int fd_ = -1;
  char *data_ = nullptr;
  std::size_t file_size_ = 0;
};

void demo_mmap_streambuf() {
  section("D. Memory-Mapped File Streambuf");

  {
    std::ofstream f("/tmp/ext_mmap_test.txt");
    f << "Hello from memory-mapped file!\n"
      << "This content is read via mmap.\n"
      << "No system calls needed for each byte.\n";
  }

  MmapStreambuf mbuf("/tmp/ext_mmap_test.txt");
  std::istream is(&mbuf);

  std::cout << "  file size: " << mbuf.file_size() << " bytes\n";

  std::string line;
  while (std::getline(is, line)) {
    std::cout << "  " << line << "\n";
  }
}

// ===========================================================================
// E. Connecting Custom Streambuf to istream/ostream
// ===========================================================================

class CounterStreambuf : public std::streambuf {
public:
  CounterStreambuf() : count_(0) {}
  int get_count() const { return count_; }
  void reset_count() { count_ = 0; }

protected:
  int_type overflow(int_type ch) override {
    if (traits_type::eof() != ch)
      ++count_;
    return ch;
  }

private:
  int count_;
};

void demo_connecting_streambuf() {
  section("E. Connecting Custom Streambuf to ostream");

  CounterStreambuf counter;
  std::ostream counter_stream(&counter);

  counter_stream << "Hello" << 42 << "World" << 3.14 << "\n";
  std::cout << "  characters written: " << counter.get_count() << "\n";

  counter.reset_count();
  counter_stream << "Short";
  std::cout << "  after reset + 'Short': " << counter.get_count() << "\n";
}

// ===========================================================================
// F. Bidirectional Streambuf (Read + Write)
// ===========================================================================

class BidirectionalStreambuf : public std::streambuf {
public:
  explicit BidirectionalStreambuf(std::size_t size = 1024) : buf_(size) {
    char *begin = buf_.data();
    char *end = buf_.data() + size;
    setg(begin, begin, begin);
    setp(begin, end);
  }

  std::string written_data() const { return std::string(pbase(), pptr()); }

  void reset_write() { setp(buf_.data(), buf_.data() + buf_.size()); }

protected:
  int_type overflow(int_type ch) override {
    if (traits_type::eof() != ch) {
      *pptr() = traits_type::to_char_type(ch);
      pbump(1);
    }
    return ch;
  }

  int_type underflow() override { return traits_type::eof(); }

private:
  std::vector<char> buf_;
};

void demo_bidirectional() {
  section("F. Bidirectional Streambuf");

  BidirectionalStreambuf bi_buf(256);
  std::ostream out(&bi_buf);

  out << "Write some data" << std::endl;
  out << "More data" << std::endl;

  std::cout << "  written: \"" << bi_buf.written_data() << "\"\n";

  bi_buf.reset_write();
  out << "After reset" << std::endl;
  std::cout << "  after reset: \"" << bi_buf.written_data() << "\"\n";
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 5: Custom streambuf ===";

  demo_anatomy();
  demo_string_buffer();
  demo_tee_logging();
  demo_mmap_streambuf();
  demo_connecting_streambuf();
  demo_bidirectional();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
