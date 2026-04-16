"""
test.py – Exercise the C++ extension module built from module.cpp.

Run after building:
    cd build && cmake .. && make example_module
    python3 src/python_binding/test.py
"""

import example_module as m

# ── Free functions ────────────────────────────────────────────────────────────

print("=== Free functions ===")
print(f"add(3, 4)          -> {m.add(3, 4)}")
print(f"multiply(2.5, 4.0) -> {m.multiply(2.5, 4.0)}")
print(f"greet('World')     -> {m.greet('World')}")
print(f"sum_list([1,2,3])  -> {m.sum_list([1.0, 2.0, 3.0])}")

# ── Calculator class ──────────────────────────────────────────────────────────

print("\n=== Calculator class ===")
calc = m.Calculator(10.0)
print(f"initial            -> {calc!r}")

calc.add(5)
print(f"after add(5)       -> {calc.value}")

calc.subtract(3)
print(f"after subtract(3)  -> {calc.value}")

calc.multiply(2)
print(f"after multiply(2)  -> {calc.value}")

calc.divide(4)
print(f"after divide(4)    -> {calc.value}")

calc.reset()
print(f"after reset()      -> {calc.value}")

# ── Exception propagation ─────────────────────────────────────────────────────

print("\n=== Exception propagation ===")
try:
    calc.divide(0)
except ValueError as e:
    print(f"Caught expected error: {e}")
