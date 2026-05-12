# examples/oer — OER encode/decode tutorial example
#
# Standalone example demonstrating Octet Encoding Rules (OER) encoding/decoding
# of a constrained integer (0–255). Shows OER's compact fixed-width output
# vs BER's tag+length prefix overhead.
#
# Usage (from project root):
#   cmake -B build -GNinja
#   ninja -C build oer-demo
#   ./build/examples/oer/oer-demo

cmake_minimum_required(VERSION 3.20)
project(oer-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(oer-demo main.cpp)

# Use pre-built asn1pp libraries and generated headers from parent build
set(ASN1PP_PARENT_BUILD_DIR "${PROJECT_SOURCE_DIR}/../../build_oer")
target_link_libraries(oer-demo PRIVATE
    "${ASN1PP_PARENT_BUILD_DIR}/libs/libasn1pp-codec.a"
    "${ASN1PP_PARENT_BUILD_DIR}/libs/arch/libasn1pp-arch.a"
)

target_include_directories(oer-demo PRIVATE
    "${PROJECT_SOURCE_DIR}/../../libs"
    "${ASN1PP_PARENT_BUILD_DIR}/generated"
)