cmake_minimum_required(VERSION 3.20)
project(asn1pp VERSION 0.1.0 LANGUAGES CXX)

# C++20 requirement
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Embedded profile options
option(ASN1PP_EMBEDDED "Embedded profile (zero heap, fixed buffers)" OFF)
option(ASN1PP_NO_TEXT_CODECS "Strip text codecs (XER/JER)" OFF)

# Enable testing
enable_testing()

# Subdirectories
add_subdirectory(libs)
add_subdirectory(src)
add_subdirectory(test)

# CMake module path for asn1pp-cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Apply embedded profile compile definitions (targets created in libs subdirectory)
if(ASN1PP_EMBEDDED)
    target_compile_definitions(asn1pp-codec PUBLIC ASN1PP_EMBEDDED ASN1PP_MAX_PDU=2048)
    target_compile_definitions(asn1pp-arch PUBLIC ASN1PP_EMBEDDED ASN1PP_MAX_PDU=2048)
endif()

if(ASN1PP_NO_TEXT_CODECS)
    target_compile_definitions(asn1pp-codec PUBLIC ASN1PP_NO_TEXT_CODECS)
    target_compile_definitions(asn1pp-arch PUBLIC ASN1PP_NO_TEXT_CODECS)
endif()