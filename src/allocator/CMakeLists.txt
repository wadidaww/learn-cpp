cmake_minimum_required(VERSION 3.10)

# ── Lesson 1: std::allocator basics ──────────────────────────────────────────
add_executable(alloc_01_std 01_std_allocator.cpp)
target_compile_options(alloc_01_std PRIVATE -Wall -Wextra -O2)

# ── Lesson 2: Custom allocator (minimal + tracking) ───────────────────────────
add_executable(alloc_02_custom 02_custom_allocator.cpp)
target_compile_options(alloc_02_custom PRIVATE -Wall -Wextra -O2)

# ── Lesson 3: Arena / pool allocator ─────────────────────────────────────────
add_executable(alloc_03_arena 03_arena_allocator.cpp)
target_compile_options(alloc_03_arena PRIVATE -Wall -Wextra -O2)

# ── Lesson 4: std::pmr polymorphic memory resources ──────────────────────────
add_executable(alloc_04_pmr 04_pmr_allocator.cpp)
target_compile_options(alloc_04_pmr PRIVATE -Wall -Wextra -O2)
target_link_libraries(alloc_04_pmr PRIVATE pthread)

# ── Lesson 5: Combining allocators across multiple containers ─────────────────
add_executable(alloc_05_combining 05_combining_containers.cpp)
target_compile_options(alloc_05_combining PRIVATE -Wall -Wextra -O2)
