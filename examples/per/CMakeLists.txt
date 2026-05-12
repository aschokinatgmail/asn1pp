# examples/per — PER (aligned + unaligned) encode/decode tutorial
#
# Standalone example demonstrating APER vs UPER encoding.
#
# Usage (from project root):
#   cmake -B build -GNinja
#   ninja -C build

cmake_minimum_required(VERSION 3.20)
project(per-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(per-example main.cpp)

# Link against pre-built static libraries
set(ASN1PP_PARENT_BUILD_DIR "${PROJECT_SOURCE_DIR}/../../build")
target_link_libraries(per-example PRIVATE
    "${ASN1PP_PARENT_BUILD_DIR}/libs/libasn1pp-codec.a"
    "${ASN1PP_PARENT_BUILD_DIR}/libs/arch/libasn1pp-arch.a"
)

target_include_directories(per-example PRIVATE
    "${PROJECT_SOURCE_DIR}/../../libs"
    "${ASN1PP_PARENT_BUILD_DIR}/generated"
)