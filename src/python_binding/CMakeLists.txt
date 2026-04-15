cmake_minimum_required(VERSION 3.14)

# Locate the Python interpreter and development headers.
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# Locate pybind11.  When pybind11 is installed as a Python package (pip) the
# cmake files live inside the package; ask Python where they are.
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c
            "import pybind11; print(pybind11.get_cmake_dir())"
    OUTPUT_VARIABLE _pybind11_cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
)
list(APPEND CMAKE_PREFIX_PATH "${_pybind11_cmake_dir}")
find_package(pybind11 REQUIRED)

# Build the extension module.  The target name must match the first argument
# of PYBIND11_MODULE() in module.cpp.
pybind11_add_module(example_module module.cpp)

target_compile_options(example_module PRIVATE -Wall -Wextra -O2)

# Copy the built .so next to test.py so the test can import it directly.
add_custom_command(TARGET example_module POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:example_module>"
        "${CMAKE_CURRENT_SOURCE_DIR}/$<TARGET_FILE_NAME:example_module>"
    COMMENT "Copying example_module to source directory"
)
