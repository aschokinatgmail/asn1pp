add_library(asn1pp-arch STATIC arch.cpp)

target_include_directories(asn1pp-arch
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(asn1pp-arch PUBLIC
    cxx_std_20
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|i386")
    target_compile_options(asn1pp-arch PRIVATE -msse4.2 -mavx -mavx2)
endif()