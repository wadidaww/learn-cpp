// Python binding example using pybind11
//
// Demonstrates how to expose C++ functions and classes to Python.
// Build with CMake, then import the resulting .so from Python.

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>   // automatic std::vector / std::map conversions
#include <string>
#include <vector>
#include <stdexcept>

namespace py = pybind11;

// ── 1. Free functions ────────────────────────────────────────────────────────

int add(int a, int b) { return a + b; }

double multiply(double a, double b) { return a * b; }

std::string greet(const std::string& name) {
    return "Hello from C++, " + name + "!";
}

// Returns the sum of every element in a list – shows std::vector <-> list
// conversion provided by <pybind11/stl.h>.
double sum_list(const std::vector<double>& values) {
    double total = 0.0;
    for (double v : values) total += v;
    return total;
}

// ── 2. A simple class ────────────────────────────────────────────────────────

class Calculator {
public:
    explicit Calculator(double initial_value = 0.0)
        : value_(initial_value) {}

    void   add(double x)      { value_ += x; }
    void   subtract(double x) { value_ -= x; }
    void   multiply(double x) { value_ *= x; }
    void   divide(double x) {
        if (x == 0.0) throw std::invalid_argument("division by zero");
        value_ /= x;
    }

    void   reset()             { value_ = 0.0; }
    double value() const       { return value_; }

    std::string __repr__() const {
        return "Calculator(value=" + std::to_string(value_) + ")";
    }

private:
    double value_;
};

// ── 3. Module definition ─────────────────────────────────────────────────────

// The first argument to PYBIND11_MODULE must match the target name in CMake.
PYBIND11_MODULE(example_module, m) {
    m.doc() = "Example C++ module exposed to Python via pybind11";

    // --- free functions ---
    m.def("add",      &add,
          py::arg("a"), py::arg("b"),
          "Return the sum of two integers.");

    m.def("multiply", &multiply,
          py::arg("a"), py::arg("b"),
          "Return the product of two doubles.");

    m.def("greet",    &greet,
          py::arg("name"),
          "Return a greeting string.");

    m.def("sum_list", &sum_list,
          py::arg("values"),
          "Return the sum of a list of floats.");

    // --- Calculator class ---
    py::class_<Calculator>(m, "Calculator",
        "A simple accumulator that wraps a C++ class.")
        .def(py::init<double>(),
             py::arg("initial_value") = 0.0,
             "Create a Calculator with an optional starting value.")
        .def("add",      &Calculator::add,      py::arg("x"), "Add x to the current value.")
        .def("subtract", &Calculator::subtract, py::arg("x"), "Subtract x from the current value.")
        .def("multiply", &Calculator::multiply, py::arg("x"), "Multiply the current value by x.")
        .def("divide",   &Calculator::divide,   py::arg("x"), "Divide the current value by x.")
        .def("reset",    &Calculator::reset,                  "Reset the value to 0.")
        .def_property_readonly("value", &Calculator::value,  "The current accumulated value.")
        .def("__repr__", &Calculator::__repr__);
}
