# examples/bare-metal — STM32F407 cross-compiled demo
#
# Standalone example that exercises asn1pp codecs on bare-metal ARM.
# Built only when BAREMETAL_DEMO=ON and cross-compiling for ARM.
#
# Usage (from project root):
#   cmake -B build_cross \
#     -GNinja \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-stm32f407.cmake \
#     -DBAREMETAL_DEMO=ON \
#     -DCMAKE_BUILD_TYPE=MinSizeRel
#   ninja -C build_cross

if(NOT BAREMETAL_DEMO OR NOT CMAKE_CROSSCOMPILING)
    message(STATUS "bare-metal demo: skipped (requires BAREMETAL_DEMO=ON and cross-compilation)")
    return()
endif()

add_executable(bare-metal-demo main.cpp)

target_link_libraries(bare-metal-demo PRIVATE asn1pp-codec asn1pp-arch)

target_include_directories(bare-metal-demo PRIVATE
    "${PROJECT_SOURCE_DIR}/libs"
    "${PROJECT_BINARY_DIR}/generated"
)

# Forward all codec definitions (ASN1PP_EMBEDDED, ASN1PP_NO_TEXT_CODECS, etc.)
get_property(_codec_defs GLOBAL PROPERTY ASN1PP_CODEC_DEFS)
target_compile_definitions(bare-metal-demo PRIVATE ${_codec_defs})

# Post-build: print section sizes
add_custom_command(TARGET bare-metal-demo POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:bare-metal-demo>
    COMMENT "Section sizes for bare-metal-demo"
)