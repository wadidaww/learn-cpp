// 10_custom_formatter.cpp
//
// Lesson 10 — Custom Formatter (C++20)
//
// std::format uses std::formatter<T> to format custom types.  By
// specializing std::formatter you can:
//   - Use your type with std::format
//   - Support custom format specifiers
//   - Integrate with the standard formatting ecosystem
//
// Key requirements for std::formatter<T>:
//   - parse(format_parse_context&) — parse format spec
//   - format(T const&, format_context&) — format the value
//
// Patterns shown:
//   A. std::formatter concept and requirements
//   B. Basic formatter for a custom type (Point)
//   C. Custom format specifiers
//   D. Integration with std::format
//   E. Formatting containers of custom types
//   F. Compile-time format string validation

#include <cmath>
#include <format>
#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char *title) {
  std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. std::formatter Concept and Requirements
// ===========================================================================

void demo_formatter_concept() {
  section("A. std::formatter Concept and Requirements");

  std::cout << "  To specialize std::formatter<T>:\n\n";

  std::cout << "  template <>\n";
  std::cout << "  struct std::formatter<MyType> {\n";
  std::cout << "    // Parse format spec (e.g., '{:.2f}', '{:10s}')\n";
  std::cout << "    constexpr auto parse(format_parse_context& ctx) {\n";
  std::cout << "      // Return iterator to end of parsed spec\n";
  std::cout << "    }\n\n";
  std::cout << "    // Format the value\n";
  std::cout
      << "    auto format(const MyType& val, format_context& ctx) const {\n";
  std::cout << "      // Write to ctx.out()\n";
  std::cout << "    }\n";
  std::cout << "  };\n\n";

  std::cout << "  Format spec syntax: {:format_spec}\n";
  std::cout << "  Common specs:\n";
  std::cout << "    fill + align: {:>10}, {:<10}, {:^10}\n";
  std::cout << "    width:        {:10}\n";
  std::cout << "    precision:    {:.2f}\n";
}

// ===========================================================================
// B. Basic Formatter for Point
// ===========================================================================

struct Point {
  double x, y;
};

template <> struct std::formatter<Point> {
  char spec_ = 'f';

  constexpr auto parse(std::format_parse_context &ctx) {
    auto it = ctx.begin();
    auto end = ctx.end();

    if (it != end && *it != '}') {
      spec_ = *it;
      ++it;
    }

    if (it != end && *it != '}') {
      throw std::format_error("invalid format spec for Point");
    }

    return it;
  }

  auto format(const Point &p, std::format_context &ctx) const {
    switch (spec_) {
    case 'f': // full: (x, y)
      return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
    case 'p': // pair: x,y
      return std::format_to(ctx.out(), "{},{}", p.x, p.y);
    case 'x': // just x
      return std::format_to(ctx.out(), "{}", p.x);
    case 'y': // just y
      return std::format_to(ctx.out(), "{}", p.y);
    case 'd': { // distance from origin
      double dist = std::sqrt(p.x * p.x + p.y * p.y);
      return std::format_to(ctx.out(), "{}", dist);
    }
    default:
      return std::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
  }
};

void demo_basic_formatter() {
  section("B. Basic Formatter for Point");

  Point p{3.0, 4.0};

  std::cout << "  default:  " << std::format("{}", p) << "\n";
  std::cout << "  {:f}:     " << std::format("{:f}", p) << "\n";
  std::cout << "  {:p}:     " << std::format("{:p}", p) << "\n";
  std::cout << "  {:x}:     " << std::format("{:x}", p) << "\n";
  std::cout << "  {:y}:     " << std::format("{:y}", p) << "\n";
  std::cout << "  {:d}:     " << std::format("{:d}", p) << "\n";
}

// ===========================================================================
// C. Custom Format Specifiers
// ===========================================================================

struct Color {
  uint8_t r, g, b;
};

template <> struct std::formatter<Color> {
  char spec_ = 'h';

  constexpr auto parse(std::format_parse_context &ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      spec_ = *it;
      ++it;
    }
    if (it == ctx.end() || *it != '}') {
      throw std::format_error("invalid format spec for Color");
    }
    return it;
  }

  auto format(const Color &c, std::format_context &ctx) const {
    switch (spec_) {
    case 'h': // hex: #RRGGBB
      return std::format_to(ctx.out(), "#{:02X}{:02X}{:02X}", c.r, c.g, c.b);
    case 'r': // rgb: (r, g, b)
      return std::format_to(ctx.out(), "rgb({}, {}, {})", c.r, c.g, c.b);
    case 'n': { // name approximation
      const char *name = "unknown";
      if (c.r > 200 && c.g < 50 && c.b < 50)
        name = "red";
      else if (c.r < 50 && c.g > 200 && c.b < 50)
        name = "green";
      else if (c.r < 50 && c.g < 50 && c.b > 200)
        name = "blue";
      else if (c.r > 200 && c.g > 200 && c.b < 50)
        name = "yellow";
      else if (c.r > 200 && c.g > 200 && c.b > 200)
        name = "white";
      else if (c.r < 50 && c.g < 50 && c.b < 50)
        name = "black";
      return std::format_to(ctx.out(), "{}", name);
    }
    default:
      return std::format_to(ctx.out(), "#{:02X}{:02X}{:02X}", c.r, c.g, c.b);
    }
  }
};

void demo_custom_specifiers() {
  section("C. Custom Format Specifiers");

  Color red{255, 0, 0};
  Color green{0, 255, 0};
  Color blue{0, 0, 255};
  Color white{255, 255, 255};

  std::cout << "  hex:     " << std::format("{:h}", red) << "\n";
  std::cout << "  rgb:     " << std::format("{:r}", red) << "\n";
  std::cout << "  name:    " << std::format("{:n}", red) << "\n";
  std::cout << "\n";

  std::cout << "  colors:\n";
  std::cout << "    " << std::format("{:n} = {:h}", red, red) << "\n";
  std::cout << "    " << std::format("{:n} = {:h}", green, green) << "\n";
  std::cout << "    " << std::format("{:n} = {:h}", blue, blue) << "\n";
  std::cout << "    " << std::format("{:n} = {:h}", white, white) << "\n";
}

// ===========================================================================
// D. Integration with std::format
// ===========================================================================

struct Duration {
  long long microseconds;
};

template <> struct std::formatter<Duration> {
  char spec_ = 'u';

  constexpr auto parse(std::format_parse_context &ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      spec_ = *it;
      ++it;
    }
    if (it == ctx.end() || *it != '}') {
      throw std::format_error("invalid format spec for Duration");
    }
    return it;
  }

  auto format(const Duration &d, std::format_context &ctx) const {
    switch (spec_) {
    case 'u': // microseconds
      return std::format_to(ctx.out(), "{}us", d.microseconds);
    case 'm': // milliseconds
      return std::format_to(ctx.out(), "{:.3}ms", d.microseconds / 1000.0);
    case 's': // seconds
      return std::format_to(ctx.out(), "{:.6}s", d.microseconds / 1000000.0);
    default:
      return std::format_to(ctx.out(), "{}us", d.microseconds);
    }
  }
};

void demo_std_format() {
  section("D. Integration with std::format");

  Duration d{1234567};

  std::cout << "  {:u} " << std::format("{:u}", d) << "\n";
  std::cout << "  {:m} " << std::format("{:m}", d) << "\n";
  std::cout << "  {:s} " << std::format("{:s}", d) << "\n";

  std::cout << "\n  std::format examples:\n";
  std::cout << "    Point: " << std::format("{}", Point{1.5, 2.5}) << "\n";
  std::cout << "    Color: " << std::format("{:h}", Color{255, 128, 0}) << "\n";
  std::cout << "    Duration: " << std::format("{:m}", Duration{500000})
            << "\n";
}

// ===========================================================================
// E. Formatting Containers of Custom Types
// ===========================================================================

struct Student {
  std::string name;
  double gpa;
};

template <> struct std::formatter<Student> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.end(); }

  auto format(const Student &s, std::format_context &ctx) const {
    return std::format_to(ctx.out(), "{} (GPA: {:.2})", s.name, s.gpa);
  }
};

void demo_container_formatting() {
  section("E. Formatting Containers of Custom Types");

  std::vector<Student> students = {
      {"Alice", 3.9}, {"Bob", 3.5}, {"Charlie", 3.7}};

  for (const auto &s : students) {
    std::cout << "  " << std::format("{}", s) << "\n";
  }

  std::cout << "\n  formatted list: ";
  std::cout << "[";
  for (size_t i = 0; i < students.size(); ++i) {
    if (i > 0)
      std::cout << ", ";
    std::cout << std::format("{}", students[i]);
  }
  std::cout << "]\n";
}

// ===========================================================================
// F. Compile-time Format String Validation
// ===========================================================================

struct Temperature {
  double celsius;
};

template <> struct std::formatter<Temperature> {
  char unit_ = 'c';

  constexpr auto parse(std::format_parse_context &ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      unit_ = *it;
      ++it;
    }
    if (it == ctx.end() || *it != '}') {
      throw std::format_error("invalid spec");
    }
    return it;
  }

  auto format(const Temperature &t, std::format_context &ctx) const {
    switch (unit_) {
    case 'c':
      return std::format_to(ctx.out(), "{:.1}C", t.celsius);
    case 'f':
      return std::format_to(ctx.out(), "{:.1}F", t.celsius * 9.0 / 5.0 + 32.0);
    case 'k':
      return std::format_to(ctx.out(), "{:.1}K", t.celsius + 273.15);
    default:
      return std::format_to(ctx.out(), "{:.1}C", t.celsius);
    }
  }
};

void demo_compile_time_validation() {
  section("F. Compile-time Format String Validation");

  Temperature t{100.0};

  // Format strings are validated at compile time
  // Uncomment to see compile error: std::format("{:z}", t);

  std::cout << "  100C = " << std::format("{:c}", t) << "\n";
  std::cout << "  100C = " << std::format("{:f}", t) << "\n";
  std::cout << "  100C = " << std::format("{:k}", t) << "\n";

  std::string msg = std::format("Water boils at {:c}", t);
  std::cout << "  " << msg << "\n";

  Temperature freezing{0.0};
  std::cout << "  temps: "
            << std::format("{:c}, {:c}, {:c}", freezing, t, Temperature{37.0})
            << "\n";
}

// ===========================================================================
// main
// ===========================================================================
int main() {
  std::cout << "=== Lesson 10: Custom Formatter (C++20) ===";

  demo_formatter_concept();
  demo_basic_formatter();
  demo_custom_specifiers();
  demo_std_format();
  demo_container_formatting();
  demo_compile_time_validation();

  std::cout << "\n=== All demos complete ===\n";
  return 0;
}
