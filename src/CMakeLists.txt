add_executable(asn1pp-gen
    gen/main.cpp
    gen/generator.cpp
    gen/schema_parser.cpp
    gen/ast_print.cpp
)

target_include_directories(asn1pp-gen
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(asn1pp-gen
    PRIVATE
        asn1pp-codec
)

target_compile_features(asn1pp-gen PRIVATE
    cxx_std_20
)