add_subdirectory(arch)

# Collect codec sources — conditionally exclude text codecs when stripped
set(_codec_sources
    # Buffer module
    buffer/buffer.cpp

    # Codec module
    codec/codec.cpp

    # BER encoder/decoder
    codec/ber/ber_encoder.cpp
    codec/ber/ber_decoder.cpp
    codec/ber/der_encoder.cpp

    # PER encoder/decoder
    codec/per/per_encoder.cpp
    codec/per/per_decoder.cpp

    # OER encoder/decoder
    codec/oer/oer_encoder.cpp
    codec/oer/oer_decoder.cpp
)

# Text codecs (XER/JER) — excluded in embedded profile or when NO_TEXT_CODECS
if(NOT NO_TEXT_CODECS AND NOT EMBEDDED)
    list(APPEND _codec_sources
        # XER encoder/decoder
        codec/xer/xer_encoder.cpp
        codec/xer/xer_decoder.cpp

        # JER encoder/decoder
        codec/jer/jer_encoder.cpp
        codec/jer/jer_decoder.cpp
    )
endif()

add_library(asn1pp-codec STATIC ${_codec_sources})

target_include_directories(asn1pp-codec
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(asn1pp-codec PRIVATE asn1pp-arch)

target_compile_features(asn1pp-codec PUBLIC
    cxx_std_20
)

# Exclude from coverage (3rd party content placeholder)
target_compile_definitions(asn1pp-codec PRIVATE
    ASN1PP_EXCLUDE_COVERAGE
)