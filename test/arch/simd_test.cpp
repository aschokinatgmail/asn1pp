#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>

#include "arch/simd.hpp"
#include "arch/scalar.hpp"

using namespace asn1pp::arch;

// ============================================================================
// Dispatch validation tests
// ============================================================================

TEST(SimdDispatch, AvailableLevelReturnsValid) {
    const auto level = available_simd_level();
    // Must be one of the defined levels
    EXPECT_TRUE(level == simd_level::scalar ||
                level == simd_level::sse42 ||
                level == simd_level::avx2 ||
                level == simd_level::neon);
}

TEST(SimdDispatch, CompileLevelAtLeastScalar) {
    // compile_level must be a valid enum (>= scalar implicitly)
    EXPECT_GE(static_cast<int>(compile_level), static_cast<int>(simd_level::scalar));
}

TEST(SimdDispatch, AvailableLevelAtLeastScalar) {
    EXPECT_GE(static_cast<int>(available_simd_level()),
              static_cast<int>(simd_level::scalar));
}

// ============================================================================
// Scalar backend tests (all paths verify against scalar explicitly)
// ============================================================================

TEST(ScalarBackend, LoadU64Be) {
    // Test with various sizes 1-8
    const uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff};

    EXPECT_EQ(scalar::load_u64_be(data, 0), 0u);
    EXPECT_EQ(scalar::load_u64_be(data, 1), 0x12u);
    EXPECT_EQ(scalar::load_u64_be(data, 2), 0x1234u);
    EXPECT_EQ(scalar::load_u64_be(data, 3), 0x123456u);
    EXPECT_EQ(scalar::load_u64_be(data, 4), 0x12345678u);
    EXPECT_EQ(scalar::load_u64_be(data, 5), 0x123456789au);
    EXPECT_EQ(scalar::load_u64_be(data, 6), 0x123456789abcu);
    EXPECT_EQ(scalar::load_u64_be(data, 7), 0x123456789abcdeu);
    EXPECT_EQ(scalar::load_u64_be(data, 8), 0x123456789abcdeffu);
}

TEST(ScalarBackend, StoreU64BeRoundtrip) {
    uint8_t buf[8] = {};
    scalar::store_u64_be(buf, 0x123456789abcdef0u, 8);
    EXPECT_EQ(scalar::load_u64_be(buf, 8), 0x123456789abcdef0u);

    scalar::store_u64_be(buf, 0x42u, 3);
    EXPECT_EQ(scalar::load_u64_be(buf, 3), 0x42u);
    EXPECT_EQ(buf[0], 0x00);
    EXPECT_EQ(buf[1], 0x00);
    EXPECT_EQ(buf[2], 0x42);
}

TEST(ScalarBackend, BatchLoadU64Be) {
    const uint8_t d1[] = {0x01, 0x02, 0x03, 0x04};
    const uint8_t d2[] = {0xaa, 0xbb};
    const uint8_t d3[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
    const uint8_t d4[] = {0xff};

    const uint8_t* ptrs[] = {d1, d2, d3, d4};
    const size_t sizes[] = {4, 2, 8, 1};
    uint64_t out[4] = {};

    scalar::batch_load_u64_be(ptrs, sizes, out, 4);

    EXPECT_EQ(out[0], 0x01020304u);
    EXPECT_EQ(out[1], 0xaabbu);
    EXPECT_EQ(out[2], 0x1020304050607080u);
    EXPECT_EQ(out[3], 0xffu);
}

TEST(ScalarBackend, CopyBytes) {
    const uint8_t src[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t dst[16] = {};

    const size_t copied = scalar::copy_bytes(dst, src, 10);
    EXPECT_EQ(copied, 10u);
    EXPECT_EQ(std::memcmp(dst, src, 10), 0);

    // Zero-length copy
    EXPECT_EQ(scalar::copy_bytes(dst, src, 0), 0u);
}

TEST(ScalarBackend, CompareTags) {
    const uint8_t tags[] = {0x02, 0x02, 0x04, 0x02, 0xff, 0x02};
    bool results[6] = {};

    scalar::compare_tags(tags, 0x02, 6, results);

    EXPECT_TRUE(results[0]);
    EXPECT_TRUE(results[1]);
    EXPECT_FALSE(results[2]);
    EXPECT_TRUE(results[3]);
    EXPECT_FALSE(results[4]);
    EXPECT_TRUE(results[5]);
}

// ============================================================================
// Dispatch consistency tests (verify dispatched path matches scalar reference)
// ============================================================================

TEST(SimdDispatch, DispatchBatchLoadMatchesScalar) {
    // Create 4 test values with mixed sizes
    const uint8_t d1[] = {0xde, 0xad, 0xbe, 0xef};
    const uint8_t d2[] = {0xca, 0xfe};
    const uint8_t d3[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    const uint8_t d4[] = {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67};

    const uint8_t* ptrs[] = {d1, d2, d3, d4};
    const size_t sizes[] = {4, 2, 8, 7};

    uint64_t scalar_out[4] = {};
    uint64_t dispatch_out[4] = {};

    scalar::batch_load_u64_be(ptrs, sizes, scalar_out, 4);
    batch_load_u64_be(ptrs, sizes, dispatch_out, 4);

    for (size_t i = 0; i < 4; ++i) {
        EXPECT_EQ(dispatch_out[i], scalar_out[i]) << "Mismatch at index " << i;
    }
}

TEST(SimdDispatch, DispatchBatchStoreMatchesScalar) {
    const uint64_t values[] = {0xdeadbeefu, 0xcafebabeu, 0x1234567890abcdefu, 0x42u};
    const size_t sizes[] = {4, 4, 8, 1};

    uint8_t scalar_buf[32] = {};
    uint8_t dispatch_buf[32] = {};

    uint8_t* scalar_ptrs[] = {scalar_buf, scalar_buf + 8, scalar_buf + 16, scalar_buf + 24};
    uint8_t* dispatch_ptrs[] = {dispatch_buf, dispatch_buf + 8, dispatch_buf + 16, dispatch_buf + 24};

    scalar::batch_store_u64_be(scalar_ptrs, values, sizes, 4);
    batch_store_u64_be(dispatch_ptrs, values, sizes, 4);

    EXPECT_EQ(std::memcmp(scalar_buf, dispatch_buf, 32), 0);
}

TEST(SimdDispatch, DispatchCopyBytesMatchesScalar) {
    constexpr size_t N = 64;
    uint8_t src[N];
    for (size_t i = 0; i < N; ++i) src[i] = static_cast<uint8_t>(i * 7 + 3);

    uint8_t scalar_dst[N] = {};
    uint8_t dispatch_dst[N] = {};

    const size_t scalar_copied = scalar::copy_bytes(scalar_dst, src, N);
    const size_t dispatch_copied = copy_bytes(dispatch_dst, src, N);

    EXPECT_EQ(scalar_copied, N);
    EXPECT_EQ(dispatch_copied, N);
    EXPECT_EQ(std::memcmp(scalar_dst, dispatch_dst, N), 0);
}

TEST(SimdDispatch, DispatchCopyBytesZero) {
    uint8_t buf[16] = {1};
    EXPECT_EQ(copy_bytes(buf, nullptr, 0), 0u);
}

TEST(SimdDispatch, DispatchCompareTagsMatchesScalar) {
    constexpr size_t N = 48;
    uint8_t tags[N];
    for (size_t i = 0; i < N; ++i) tags[i] = static_cast<uint8_t>(i % 3 == 0 ? 0x02 : 0x04);

    bool scalar_results[N] = {};
    bool dispatch_results[N] = {};

    scalar::compare_tags(tags, 0x02, N, scalar_results);
    compare_tags(tags, 0x02, N, dispatch_results);

    for (size_t i = 0; i < N; ++i) {
        EXPECT_EQ(dispatch_results[i], scalar_results[i]) << "Mismatch at index " << i;
    }
}

TEST(SimdDispatch, DispatchCompareTagsEmpty) {
    bool result = false;
    compare_tags(nullptr, 0x02, 0, &result);
    // No-op for zero count, just verify no crash
}

// ============================================================================
// Single-value dispatch tests (always scalar)
// ============================================================================

TEST(SimdDispatch, DispatchLoadU64BeIsScalar) {
    const uint8_t data[] = {0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe};
    EXPECT_EQ(load_u64_be(data, 8), scalar::load_u64_be(data, 8));
    EXPECT_EQ(load_u64_be(data, 4), scalar::load_u64_be(data, 4));
    EXPECT_EQ(load_u64_be(data, 0), 0u);
}

TEST(SimdDispatch, DispatchStoreU64BeIsScalar) {
    uint8_t arch_buf[8] = {};
    uint8_t scalar_buf[8] = {};
    store_u64_be(arch_buf, 0x12345678u, 4);
    scalar::store_u64_be(scalar_buf, 0x12345678u, 4);
    EXPECT_EQ(std::memcmp(arch_buf, scalar_buf, 8), 0);
}
