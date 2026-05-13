# examples/asn1-to-cpp/ — ASN.1 schema to generated C++ round-trip example
#
# Standalone example demonstrating the full workflow:
#   ASN.1 schema -> generated C++ header -> BER encode/decode round-trip.
#
# Usage (from this directory):
#   cmake -B build -GNinja
#   ninja -C build
#   ./build/asn1-to-cpp-demo

cmake_minimum_required(VERSION 3.20)
project(asn1-to-cpp-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(asn1-to-cpp-demo main.cpp)

# The generated header includes the public installed-form paths. Provide local
# shims so this example remains standalone from the source tree.
set(ASN1PP_SHIM_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${ASN1PP_SHIM_INCLUDE_DIR}/asn1pp")
file(WRITE "${ASN1PP_SHIM_INCLUDE_DIR}/asn1pp/codec.hpp"
    "#pragma once\n"
    "#include \"codec/codec_interface.hpp\"\n"
    "using asn1pp::asn1_tag;\n"
    "using asn1pp::error_code;\n"
    "using asn1pp::make_universal;\n"
    "using asn1pp::result;\n"
    "using asn1pp::universal_tag;\n"
)
file(WRITE "${ASN1PP_SHIM_INCLUDE_DIR}/asn1pp/traits.hpp"
    "#pragma once\n"
    "#include \"codec/traits.hpp\"\n"
)

# Link against pre-built static libraries from the parent project build.
set(ASN1PP_PARENT_BUILD_DIR "${PROJECT_SOURCE_DIR}/../../build")
target_link_libraries(asn1-to-cpp-demo PRIVATE
    "${ASN1PP_PARENT_BUILD_DIR}/libs/libasn1pp-codec.a"
    "${ASN1PP_PARENT_BUILD_DIR}/libs/arch/libasn1pp-arch.a"
)

target_include_directories(asn1-to-cpp-demo PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${ASN1PP_SHIM_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/../../libs"
    "${ASN1PP_PARENT_BUILD_DIR}/generated"
)
