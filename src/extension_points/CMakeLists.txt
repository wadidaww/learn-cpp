cmake_minimum_required(VERSION 3.10)

# ── Lesson 1: Custom Deleters for Smart Pointers ─────────────────────────────
add_executable(ext_01_deleters 01_custom_deleters.cpp)
target_compile_options(ext_01_deleters PRIVATE -Wall -Wextra -O2)

# ── Lesson 2: Custom Iterators ───────────────────────────────────────────────
add_executable(ext_02_iterators 02_custom_iterators.cpp)
target_compile_options(ext_02_iterators PRIVATE -Wall -Wextra -O2)

# ── Lesson 3: Traits Specialization ──────────────────────────────────────────
add_executable(ext_03_traits 03_traits_specialization.cpp)
target_compile_options(ext_03_traits PRIVATE -Wall -Wextra -O2)

# ── Lesson 4: C++20 Concepts ─────────────────────────────────────────────────
add_executable(ext_04_concepts 04_cpp20_concepts.cpp)
target_compile_options(ext_04_concepts PRIVATE -Wall -Wextra -O2)

# ── Lesson 5: Custom streambuf ───────────────────────────────────────────────
add_executable(ext_05_streambuf 05_streambuf_customization.cpp)
target_compile_options(ext_05_streambuf PRIVATE -Wall -Wextra -O2)

# ── Lesson 6: Custom Error Category ──────────────────────────────────────────
add_executable(ext_06_errors 06_error_category.cpp)
target_compile_options(ext_06_errors PRIVATE -Wall -Wextra -O2)

# ── Lesson 7: Ranges and Views (C++20) ──────────────────────────────────────
add_executable(ext_07_ranges 07_ranges_views.cpp)
target_compile_options(ext_07_ranges PRIVATE -Wall -Wextra -O2)

# ── Lesson 8: Type Erasure ──────────────────────────────────────────────────
add_executable(ext_08_type_erasure 08_type_erasure.cpp)
target_compile_options(ext_08_type_erasure PRIVATE -Wall -Wextra -O2)

# ── Lesson 9: Policy-Based Design ────────────────────────────────────────────
add_executable(ext_09_policies 09_policy_based_design.cpp)
target_compile_options(ext_09_policies PRIVATE -Wall -Wextra -O2)
target_link_libraries(ext_09_policies PRIVATE pthread)

# ── Lesson 10: Custom Formatter (C++20) ──────────────────────────────────────
add_executable(ext_10_formatter 10_custom_formatter.cpp)
target_compile_options(ext_10_formatter PRIVATE -Wall -Wextra -O2)
