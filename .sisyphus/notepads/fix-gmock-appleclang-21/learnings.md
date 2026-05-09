# GMock AppleClang 21 Build Fix - Learnings

## Problem
`error: use of undeclared identifier 'GTEST_INTERNAL_ATTRIBUTE_MAYBE_UNUSED'` when building googletest v1.16.0 with AppleClang 21.

## Root Cause
1. A system-installed GTest at `/usr/local/include/gtest/` was taking precedence over the fetched GTest headers.
2. AppleClang 21 has issues with the `__attribute__((maybe_unused))` style attribute (GCC-style) on function parameters.
3. The `MATCHER` macro uses `GTEST_INTERNAL_ATTRIBUTE_MAYBE_UNUSED` on the `result_listener` parameter, causing `-Wunused-parameter` errors treated as errors.

## Solution
Two-part fix applied AFTER `FetchContent_MakeAvailable(googletest)`:

```cmake
# Fix 1: Replace GCC attribute with C++11 standard attribute
target_compile_definitions(gmock PRIVATE
    "GTEST_INTERNAL_ATTRIBUTE_MAYBE_UNUSED=[[maybe_unused]]"
)

# Fix 2: Suppress unused-parameter warning since [[maybe_unused]] doesn't silence it for parameters in AppleClang 21
target_compile_options(gmock PRIVATE -Wno-unused-parameter)
```

## Key Insights
- `CMAKE_INCLUDE_PATH` and `-isystem` flags did NOT work because the system gtest was still found first
- Including fetched headers with `include_directories(BEFORE SYSTEM ...)` did NOT override system includes during GTest's own build
- The actual fix is to modify how gmock compiles, not to change include order
- GTest main branch was incompatible (different API changes), so we kept v1.16.0 with the workaround

## What Didn't Work
- `CMAKE_INCLUDE_PATH` prepending
- `-isystem` compile options
- `include_directories(BEFORE SYSTEM ...)` 
- GTest main branch (API incompatibilities)
- Defining `GTEST_INTERNAL_ATTRIBUTE_MAYBE_UNUSED=` (empty - still got unused-param warnings)
- Defining `GTEST_INTERNAL_ATTRIBUTE_MAYBE_UNUSED=__attribute__((maybe_unused))` (unknown attribute error)
