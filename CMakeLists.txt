cmake_minimum_required(VERSION 3.16)
project(riscv-sim CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Lógica del simulador, compartida por el ejecutable y las pruebas.
add_library(riscv-core STATIC
    src/memory.cpp
    src/cpu.cpp
)
target_include_directories(riscv-core PUBLIC include)

add_executable(riscv-sim src/main.cpp)
target_link_libraries(riscv-sim PRIVATE riscv-core)

enable_testing()
add_executable(test_state tests/test_state.cpp)
target_link_libraries(test_state PRIVATE riscv-core)
add_test(NAME test_state COMMAND test_state)
