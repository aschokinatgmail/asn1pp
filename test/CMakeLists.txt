# GoogleTest setup via FetchContent
include(FetchContent)

# Use v1.16.0 with manual fix for AppleClang 21 compatibility
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
)

# GTest does not need installation, and we build GMock as part of the same repo
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Suppress char8_t conversion error (clang C++20 issue)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=character-conversion" CACHE STRING "" FORCE)

FetchContent_MakeAvailable(googletest)

# Fix AppleClang 21 compatibility: define GTEST_INTERNAL_ATTRIBUTE_MAYBE_UNUSED if not defined
# AppleClang 21 doesn't properly support the maybe_unused attribute on parameters
# Use [[maybe_unused]] C++11 attribute instead which works on AppleClang 21
target_compile_definitions(gmock PRIVATE
    "GTEST_INTERNAL_ATTRIBUTE_MAYBE_UNUSED=[[maybe_unused]]"
)

# Also suppress unused-parameter warning from MATCHER macros
target_compile_options(gmock PRIVATE -Wno-unused-parameter)

# Enable testing
enable_testing()

# GoogleTest module for gtest_discover_tests
include(GoogleTest)

# CMake function to add a test executable
# Usage: asn1pp_add_test(test_name source1.cpp source2.cpp ...)
function(asn1pp_add_test test_name)
    cmake_parse_arguments(PARSE_ARGV 2 "" "" "" "")
    set(sources ${ARGN})
    if(NOT sources)
        message(FATAL_ERROR "asn1pp_add_test requires at least one source file")
    endif()

    add_executable(${test_name} ${sources})
    target_link_libraries(${test_name}
        PRIVATE
            GTest::gtest
            GTest::gmock
            GTest::gtest_main
            asn1pp-codec
    )
    target_compile_features(${test_name} PRIVATE cxx_std_20)
    target_compile_options(${test_name} PRIVATE -Wno-sign-compare -Wno-character-conversion)

    gtest_discover_tests(${test_name})
endfunction()

# Main test executable
asn1pp_add_test(smoke_test main.cpp smoke_test.cpp)

# Buffer module tests
asn1pp_add_test(buffer_view_test buffer/buffer_view_test.cpp)
asn1pp_add_test(bit_ops_test buffer/bit_ops_test.cpp)

# Codec module tests
asn1pp_add_test(traits_test codec/traits_test.cpp)
asn1pp_add_test(tag_mapping_test codec/tag_mapping_test.cpp)
asn1pp_add_test(result_test codec/result_test.cpp)
asn1pp_add_test(codec_interface_test codec/codec_interface_test.cpp)

# Gen module tests
asn1pp_add_test(ast_test gen/ast_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/ast_print.cpp)
asn1pp_add_test(diagnostics_test gen/diagnostics_test.cpp)
asn1pp_add_test(lexer_test gen/lexer_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/lexer.cpp)
asn1pp_add_test(parser_test gen/parser_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/lexer.cpp ${CMAKE_SOURCE_DIR}/src/gen/parser.cpp ${CMAKE_SOURCE_DIR}/src/gen/ast_print.cpp)
asn1pp_add_test(EmitterBoolean_test gen/emitter_boolean_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_boolean.cpp)
asn1pp_add_test(emitter_sequence_test gen/emitter_sequence_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_sequence.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_enumerated.cpp)
asn1pp_add_test(emitter_string_test gen/emitter_string_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_string.cpp)