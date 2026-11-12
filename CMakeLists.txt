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
    src/decoder.cpp
    src/cli.cpp
)
target_include_directories(riscv-core PUBLIC include)

add_executable(riscv-sim src/main.cpp)
target_link_libraries(riscv-sim PRIVATE riscv-core)

enable_testing()
add_executable(test_state tests/test_state.cpp)
target_link_libraries(test_state PRIVATE riscv-core)
add_test(NAME test_state COMMAND test_state)

add_executable(test_decoder tests/test_decoder.cpp)
target_link_libraries(test_decoder PRIVATE riscv-core)
add_test(NAME test_decoder COMMAND test_decoder)

add_executable(test_execute tests/test_execute.cpp)
target_link_libraries(test_execute PRIVATE riscv-core)
target_compile_definitions(test_execute PRIVATE PROGRAMS_DIR="${CMAKE_SOURCE_DIR}/programs")
add_test(NAME test_execute COMMAND test_execute)

add_executable(test_cli tests/test_cli.cpp)
target_link_libraries(test_cli PRIVATE riscv-core)
target_compile_definitions(test_cli PRIVATE PROGRAMS_DIR="${CMAKE_SOURCE_DIR}/programs")
add_test(NAME test_cli COMMAND test_cli)
