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

# CMake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ── KConfig-based configuration ────────────────────────────────────────────

include(kconfig)
kconfig_parse("${CMAKE_CURRENT_SOURCE_DIR}/Kconfig")

# ── Version information ────────────────────────────────────────────────────

# Extract version from project()
set(ASN1PP_VERSION "${PROJECT_VERSION}")
set(ASN1PP_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(ASN1PP_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(ASN1PP_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

# Git revision (optional — no failure if not in a git repo)
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE ASN1PP_GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT ASN1PP_GIT_SHA)
    set(ASN1PP_GIT_SHA "unknown")
endif()

# Git dirty state
execute_process(
    COMMAND git diff --quiet
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE _dirty_exit
    ERROR_QUIET
)
if(_dirty_exit EQUAL 0)
    set(ASN1PP_GIT_DIRTY "")
else()
    set(ASN1PP_GIT_DIRTY "-dirty")
endif()

# Generate version.hpp from template
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libs/codec/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/codec/version.hpp"
    @ONLY
)

# Generate kconfig.hpp from Kconfig state
kconfig_generate("${CMAKE_CURRENT_BINARY_DIR}/generated/kconfig.hpp")

# ── Target compile definitions (derived from Kconfig) ───────────────────────

# These macros maintain backward compatibility with existing source
# code that uses ASN1PP_* names. The Kconfig-generated config.hpp
# also provides CONFIG_* macros for new code.
set(_codec_defs "")
set(_codec_defs "ASN1PP_VERSION_MAJOR=${ASN1PP_VERSION_MAJOR}")
list(APPEND _codec_defs "ASN1PP_VERSION_MINOR=${ASN1PP_VERSION_MINOR}")
list(APPEND _codec_defs "ASN1PP_VERSION_PATCH=${ASN1PP_VERSION_PATCH}")

if(EMBEDDED)
    list(APPEND _codec_defs "ASN1PP_EMBEDDED")
    list(APPEND _codec_defs "ASN1PP_NO_TEXT_CODECS")
    list(APPEND _codec_defs "ASN1PP_MAX_PDU=${MAX_PDU}")
endif()

if(NO_TEXT_CODECS)
    list(APPEND _codec_defs "ASN1PP_NO_TEXT_CODECS")
endif()

if(NOT MAX_PDU EQUAL 2048)
    list(APPEND _codec_defs "ASN1PP_MAX_PDU=${MAX_PDU}")
endif()

# Store for examples/ subdirectory consumption
set_property(GLOBAL PROPERTY ASN1PP_CODEC_DEFS "${_codec_defs}")

# ── Build targets ──────────────────────────────────────────────────────────

# Enable testing (controlled by Kconfig BUILD_TESTS)
if(BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(libs)

if(BUILD_GEN)
    add_subdirectory(src)
endif()

if(BUILD_TESTS)
    add_subdirectory(test)
endif()

# ── Apply compile definitions to library targets ────────────────────────────

target_compile_definitions(asn1pp-codec PUBLIC ${_codec_defs})
target_compile_definitions(asn1pp-codec PUBLIC
    "ASN1PP_INCLUDE_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/libs\"")
target_include_directories(asn1pp-codec PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/generated")

target_compile_definitions(asn1pp-arch PUBLIC ${_codec_defs})
target_include_directories(asn1pp-arch PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}/generated")

# ── Examples (opt-in) ────────────────────────────────────────────────────────

if(BAREMETAL_DEMO)
    add_subdirectory(examples)
endif()

# ── Final configuration dump ───────────────────────────────────────────────

kconfig_dump()
message(STATUS "asn1pp version: ${ASN1PP_VERSION} (${ASN1PP_GIT_SHA}${ASN1PP_GIT_DIRTY})")
