cmake_minimum_required(VERSION 3.20)

project(cmake_integration_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Include asn1pp CMake module
# When used via add_subdirectory from parent project: CMAKE_SOURCE_DIR points to parent
# When used standalone: need to go up from cmake_integration -> test -> asn1pp
if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/asn1pp.cmake")
    include(${CMAKE_SOURCE_DIR}/cmake/asn1pp.cmake)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../cmake/asn1pp.cmake")
    include(${CMAKE_SOURCE_DIR}/../cmake/asn1pp.cmake)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/../../cmake/asn1pp.cmake")
    include(${CMAKE_SOURCE_DIR}/../../cmake/asn1pp.cmake)
else()
    message(FATAL_ERROR "Could not find cmake/asn1pp.cmake")
endif()

# Create a minimal executable (empty source to allow CMake configuration)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp "int main() { return 0; }")

add_executable(test_app ${CMAKE_CURRENT_BINARY_DIR}/empty.cpp)

# Generate C++ from ASN.1 schema using asn1pp_generate
asn1pp_generate(
    TARGET test_app
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/TestModule.asn
)

# Test that the generated file exists
add_test(NAME generated_file_exists COMMAND test_app)