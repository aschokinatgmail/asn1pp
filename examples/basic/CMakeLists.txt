# examples/basic/ — Minimal BER encode/decode "hello world" example
#
# Standalone example demonstrating basic BER encoding and decoding.
#
# Usage (from project root):
#   cmake -B build -GNinja
#   ninja -C build
#   ./build/examples/basic/basic-demo

cmake_minimum_required(VERSION 3.20)
project(basic-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(basic-demo main.cpp)

# Link against pre-built static libraries
set(ASN1PP_PARENT_BUILD_DIR "${PROJECT_SOURCE_DIR}/../../build")
target_link_libraries(basic-demo PRIVATE
    "${ASN1PP_PARENT_BUILD_DIR}/libs/libasn1pp-codec.a"
    "${ASN1PP_PARENT_BUILD_DIR}/libs/arch/libasn1pp-arch.a"
)

target_include_directories(basic-demo PRIVATE
    "${PROJECT_SOURCE_DIR}/../../libs"
    "${ASN1PP_PARENT_BUILD_DIR}/generated"
)