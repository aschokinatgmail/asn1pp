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
asn1pp_add_test(version_test codec/version_test.cpp)

# BER module tests
asn1pp_add_test(tlv_test codec/ber/tlv_test.cpp)
asn1pp_add_test(encoder_test codec/ber/encoder_test.cpp)
asn1pp_add_test(decoder_test codec/ber/decoder_test.cpp)
asn1pp_add_test(der_encoder_test codec/ber/der_encoder_test.cpp)

# Gen module tests
asn1pp_add_test(ast_test gen/ast_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/ast_print.cpp)
asn1pp_add_test(diagnostics_test gen/diagnostics_test.cpp)
asn1pp_add_test(lexer_test gen/lexer_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/lexer.cpp)
asn1pp_add_test(parser_test gen/parser_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/lexer.cpp ${CMAKE_SOURCE_DIR}/src/gen/parser.cpp ${CMAKE_SOURCE_DIR}/src/gen/ast_print.cpp)
asn1pp_add_test(EmitterBoolean_test gen/emitter_boolean_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_boolean.cpp)
asn1pp_add_test(emitter_integer_test gen/emitter_integer_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_integer.cpp ${CMAKE_SOURCE_DIR}/src/gen/lexer.cpp ${CMAKE_SOURCE_DIR}/src/gen/parser.cpp ${CMAKE_SOURCE_DIR}/src/gen/ast_print.cpp)
asn1pp_add_test(emitter_enumerated_test gen/emitter_enumerated_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_enumerated.cpp)
asn1pp_add_test(emitter_sequence_test gen/emitter_sequence_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_sequence.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_enumerated.cpp)
asn1pp_add_test(emitter_string_test gen/emitter_string_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_string.cpp)
asn1pp_add_test(emitter_choice_test gen/emitter_choice_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_choice.cpp)
asn1pp_add_test(EmitterOid_test gen/emitter_oid_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_oid.cpp)
asn1pp_add_test(emitter_sequence_of_test gen/emitter_sequence_of_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_sequence_of.cpp ${CMAKE_SOURCE_DIR}/src/gen/lexer.cpp ${CMAKE_SOURCE_DIR}/src/gen/parser.cpp ${CMAKE_SOURCE_DIR}/src/gen/ast_print.cpp)
asn1pp_add_test(per_meta_test gen/emitter_per_meta_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_per_meta.cpp)
asn1pp_add_test(emitter_time_test gen/emitter_time_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_time.cpp)
asn1pp_add_test(emitter_ioc_test gen/emitter_ioc_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_ioc.cpp)

# Tagged types tests
asn1pp_add_test(tagged_codec_test codec/tagged_codec_test.cpp)
asn1pp_add_test(emitter_tagged_test gen/emitter_tagged_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_tagged.cpp)

# Parameterized types tests
asn1pp_add_test(parser_parameterized_test gen/parser_parameterized_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/lexer.cpp ${CMAKE_SOURCE_DIR}/src/gen/parser.cpp ${CMAKE_SOURCE_DIR}/src/gen/ast_print.cpp)
asn1pp_add_test(emitter_parameterized_test gen/emitter_parameterized_test.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_parameterized.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_sequence.cpp ${CMAKE_SOURCE_DIR}/src/gen/emitter_enumerated.cpp)

# Integration tests
asn1pp_add_test(der_roundtrip_test integration/der_roundtrip_test.cpp)

# End-to-end tests
asn1pp_add_test(x509_der_test e2e/x509_der_test.cpp)

# End-to-end tests
asn1pp_add_test(rrc_per_test e2e/rrc_per_test.cpp)

# Arch / SIMD tests
asn1pp_add_test(simd_test arch/simd_test.cpp)
asn1pp_add_test(arch_codec_test arch/arch_codec_test.cpp)

# Embedded profile tests (only when ASN1PP_EMBEDDED is enabled)
if(ASN1PP_EMBEDDED)
    asn1pp_add_test(embedded_test embedded/embedded_test.cpp)
endif()