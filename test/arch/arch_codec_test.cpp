#include <gtest/gtest.h>

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>

#include "arch/scalar.hpp"
#include "arch/simd.hpp"
#include "codec/arch_codec.hpp"
#include "codec/batch_buffer.hpp"
#include "codec/ber/decoder.hpp"
#include "buffer/buffer_view.hpp"

// ============================================================================
// 1. Scalar backend tests — verify the reference implementation
// ============================================================================

TEST(ScalarBackend, LoadU64Be) {
    const uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xff};

    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(data, 0), 0u);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(data, 1), 0x12u);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(data, 2), 0x1234u);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(data, 3), 0x123456u);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(data, 4), 0x12345678u);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(data, 5), 0x123456789au);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(data, 6), 0x123456789abcu);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(data, 7), 0x123456789abcdeu);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(data, 8), 0x123456789abcdeffu);
}

TEST(ScalarBackend, StoreU64BeRoundtrip) {
    uint8_t buf[8] = {};

    // Full 8-byte roundtrip
    asn1pp::arch::scalar::store_u64_be(buf, 0x123456789abcdef0u, 8);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(buf, 8), 0x123456789abcdef0u);

    // Partial 3-byte roundtrip
    asn1pp::arch::scalar::store_u64_be(buf, 0x42u, 3);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(buf, 3), 0x42u);
    EXPECT_EQ(buf[0], 0x00);
    EXPECT_EQ(buf[1], 0x00);
    EXPECT_EQ(buf[2], 0x42);

    // Single byte roundtrip
    asn1pp::arch::scalar::store_u64_be(buf, 0xabu, 1);
    EXPECT_EQ(asn1pp::arch::scalar::load_u64_be(buf, 1), 0xabu);
    EXPECT_EQ(buf[0], 0xab);
}

TEST(ScalarBackend, BatchLoadU64Be) {
    const uint8_t d1[] = {0x01, 0x02, 0x03, 0x04};
    const uint8_t d2[] = {0xaa, 0xbb};
    const uint8_t d3[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};
    const uint8_t d4[] = {0xff};

    const uint8_t* ptrs[] = {d1, d2, d3, d4};
    const size_t sizes[] = {4, 2, 8, 1};
    uint64_t out[4] = {};

    asn1pp::arch::scalar::batch_load_u64_be(ptrs, sizes, out, 4);

    EXPECT_EQ(out[0], 0x01020304u);
    EXPECT_EQ(out[1], 0xaabbu);
    EXPECT_EQ(out[2], 0x1020304050607080u);
    EXPECT_EQ(out[3], 0xffu);
}

TEST(ScalarBackend, CopyBytes) {
    const uint8_t src[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t dst[16] = {};

    const size_t copied = asn1pp::arch::scalar::copy_bytes(dst, src, 10);
    EXPECT_EQ(copied, 10u);
    EXPECT_EQ(std::memcmp(dst, src, 10), 0);

    // Zero-length copy
    EXPECT_EQ(asn1pp::arch::scalar::copy_bytes(dst, src, 0), 0u);
}

TEST(ScalarBackend, CompareTags) {
    const uint8_t tags[] = {0x02, 0x02, 0x04, 0x02, 0xff, 0x02};
    bool results[6] = {};

    asn1pp::arch::scalar::compare_tags(tags, 0x02, 6, results);

    EXPECT_TRUE(results[0]);
    EXPECT_TRUE(results[1]);
    EXPECT_FALSE(results[2]);
    EXPECT_TRUE(results[3]);
    EXPECT_FALSE(results[4]);
    EXPECT_TRUE(results[5]);
}

// ============================================================================
// 2. SIMD dispatch validation tests
// ============================================================================

using asn1pp::arch::simd_level;
using namespace asn1pp::arch;

TEST(SimdDispatch, AvailableLevelReturnsValid) {
    const auto level = available_simd_level();
    EXPECT_TRUE(level == simd_level::scalar ||
                level == simd_level::sse42 ||
                level == simd_level::avx2 ||
                level == simd_level::neon);
}

TEST(SimdDispatch, CompileLevelAtLeastScalar) {
    EXPECT_GE(static_cast<int>(compile_level), static_cast<int>(simd_level::scalar));
}

TEST(SimdDispatch, AvailableLevelAtLeastScalar) {
    EXPECT_GE(static_cast<int>(available_simd_level()),
              static_cast<int>(simd_level::scalar));
}

// ============================================================================
// 3. Dispatch batch consistency tests
//    (verify dispatched path matches scalar reference on all platforms)
// ============================================================================

TEST(SimdDispatch, DispatchBatchLoadMatchesScalar) {
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
    // No-op for zero count — just verify no crash
}

// ============================================================================
// 4. arch_codec batch operations tests
// ============================================================================

TEST(ArchCodec, BatchDecodeIntegers) {
    const uint8_t buf_127[] = {0x7F};
    const uint8_t buf_256[] = {0x01, 0x00};
    const uint8_t buf_large[] = {0x7F, 0xFF, 0xFF};
    const uint8_t buf_one[] = {0x00, 0x00, 0x00, 0x01};

    const uint8_t* bufs[] = {buf_127, buf_256, buf_large, buf_one};
    const size_t sizes[] = {1, 2, 3, 4};
    int64_t out[4] = {};
    asn1pp::error_code errors[4] = {};

    asn1pp::arch_codec::batch_decode_integers(bufs, sizes, out, errors, 4);

    EXPECT_EQ(errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(errors[1], asn1pp::error_code::ok);
    EXPECT_EQ(errors[2], asn1pp::error_code::ok);
    EXPECT_EQ(errors[3], asn1pp::error_code::ok);

    EXPECT_EQ(out[0], 127);
    EXPECT_EQ(out[1], 256);
    EXPECT_EQ(out[2], 8388607);
    EXPECT_EQ(out[3], 1);
}

TEST(ArchCodec, BatchEncodeIntegersRoundtrip) {
    const int64_t values[] = {127, 256, 8388607, 1};
    const size_t sizes[] = {1, 2, 3, 4};

    uint8_t out_bufs[1 + 2 + 3 + 4];
    uint8_t* bufs[] = {
        out_bufs,
        out_bufs + 1,
        out_bufs + 1 + 2,
        out_bufs + 1 + 2 + 3
    };
    asn1pp::error_code encode_errors[4] = {};

    asn1pp::arch_codec::batch_encode_integers(values, sizes, bufs, encode_errors, 4);

    EXPECT_EQ(encode_errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(encode_errors[1], asn1pp::error_code::ok);
    EXPECT_EQ(encode_errors[2], asn1pp::error_code::ok);
    EXPECT_EQ(encode_errors[3], asn1pp::error_code::ok);

    // Decode back
    const uint8_t* decode_ptrs[] = {
        out_bufs,
        out_bufs + 1,
        out_bufs + 1 + 2,
        out_bufs + 1 + 2 + 3
    };
    int64_t roundtrip[4] = {};
    asn1pp::error_code decode_errors[4] = {};

    asn1pp::arch_codec::batch_decode_integers(decode_ptrs, sizes, roundtrip, decode_errors, 4);

    EXPECT_EQ(decode_errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(decode_errors[1], asn1pp::error_code::ok);
    EXPECT_EQ(decode_errors[2], asn1pp::error_code::ok);
    EXPECT_EQ(decode_errors[3], asn1pp::error_code::ok);

    EXPECT_EQ(roundtrip[0], 127);
    EXPECT_EQ(roundtrip[1], 256);
    EXPECT_EQ(roundtrip[2], 8388607);
    EXPECT_EQ(roundtrip[3], 1);
}

TEST(ArchCodec, BatchEncodeIntegersValueOutOfRange) {
    // 128 won't fit in 1 signed byte (range -128..127)
    const int64_t values[] = {128};
    const size_t sizes[] = {1};
    uint8_t buf[1] = {};
    uint8_t* bufs[] = {buf};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_encode_integers(values, sizes, bufs, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::buffer_overflow);
}

TEST(ArchCodec, BatchEncodeIntegersNegativeFits) {
    // -128 fits in 1 signed byte
    const int64_t values[] = {-128};
    const size_t sizes[] = {1};
    uint8_t buf[1] = {};
    uint8_t* bufs[] = {buf};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_encode_integers(values, sizes, bufs, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(buf[0], 0x80);
}

TEST(ArchCodec, BatchCompareTags) {
    const uint8_t tag_bytes[] = {0x02, 0x02, 0x04, 0x02, 0xff, 0x02};
    bool results[6] = {};

    asn1pp::arch_codec::batch_compare_tags(tag_bytes, 0x02, 6, results);

    EXPECT_TRUE(results[0]);
    EXPECT_TRUE(results[1]);
    EXPECT_FALSE(results[2]);
    EXPECT_TRUE(results[3]);
    EXPECT_FALSE(results[4]);
    EXPECT_TRUE(results[5]);
}

TEST(ArchCodec, BatchDecodeZeroLengthIsError) {
    const uint8_t buf[1] = {0x42};
    const uint8_t* bufs[] = {buf};
    const size_t sizes[] = {0};  // n=0 → invalid_length
    int64_t out[1] = {};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_decode_integers(bufs, sizes, out, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::invalid_length);
    EXPECT_EQ(out[0], 0);
}

// ============================================================================
// 5. Edge cases: count=0, count=1, mixed sizes, sign extension
// ============================================================================

TEST(ArchCodec, BatchDecodeCountZero) {
    // Zero count should be a no-op
    asn1pp::arch_codec::batch_decode_integers(nullptr, nullptr, nullptr, nullptr, 0);
}

TEST(ArchCodec, BatchDecodeCountOne) {
    // Single element below SIMD width — still should work via dispatch
    const uint8_t buf[] = {0x01, 0x02, 0x03};
    const uint8_t* bufs[] = {buf};
    const size_t sizes[] = {3};
    int64_t out[1] = {};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_decode_integers(bufs, sizes, out, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(out[0], 0x010203);
}

TEST(ArchCodec, BatchDecodeMixedSizes) {
    // 1-byte, 4-byte, 8-byte, 2-byte
    const uint8_t b1[] = {0x7F};
    const uint8_t b4[] = {0x12, 0x34, 0x56, 0x78};
    const uint8_t b8[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42};
    const uint8_t b2[] = {0x12, 0x34};

    const uint8_t* bufs[] = {b1, b4, b8, b2};
    const size_t sizes[] = {1, 4, 8, 2};
    int64_t out[4] = {};
    asn1pp::error_code errors[4] = {};

    asn1pp::arch_codec::batch_decode_integers(bufs, sizes, out, errors, 4);

    EXPECT_EQ(errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(errors[1], asn1pp::error_code::ok);
    EXPECT_EQ(errors[2], asn1pp::error_code::ok);
    EXPECT_EQ(errors[3], asn1pp::error_code::ok);

    EXPECT_EQ(out[0], 127);
    EXPECT_EQ(out[1], 0x12345678);
    EXPECT_EQ(out[2], 66);
    EXPECT_EQ(out[3], 0x1234);
}

TEST(ArchCodec, BatchDecodeSignExtensionNegative) {
    // -1 as a 1-byte value: 0xFF
    const uint8_t buf[] = {0xFF};
    const uint8_t* bufs[] = {buf};
    const size_t sizes[] = {1};
    int64_t out[1] = {};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_decode_integers(bufs, sizes, out, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(out[0], -1);
}

TEST(ArchCodec, BatchDecodeSignExtensionPositive) {
    const uint8_t buf[] = {0x40, 0x00};
    const uint8_t* bufs[] = {buf};
    const size_t sizes[] = {2};
    int64_t out[1] = {};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_decode_integers(bufs, sizes, out, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(out[0], 16384);
}

TEST(ArchCodec, BatchDecodeBufferOverflow) {
    const uint8_t buf[9] = {0};
    const uint8_t* bufs[] = {buf};
    const size_t sizes[] = {9};  // > 8 → buffer_overflow
    int64_t out[1] = {};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_decode_integers(bufs, sizes, out, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::buffer_overflow);
    EXPECT_EQ(out[0], 0);
}

TEST(ArchCodec, BatchEncodeZeroSizeIsError) {
    const int64_t values[] = {42};
    const size_t sizes[] = {0};
    uint8_t* bufs[] = {nullptr};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_encode_integers(values, sizes, bufs, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::invalid_length);
}

TEST(ArchCodec, BatchCompareTagsAllMatching) {
    const uint8_t tags[] = {0x02, 0x02, 0x02, 0x02};
    bool results[4] = {};

    asn1pp::arch_codec::batch_compare_tags(tags, 0x02, 4, results);

    for (size_t i = 0; i < 4; ++i) {
        EXPECT_TRUE(results[i]);
    }
}

TEST(ArchCodec, BatchCompareTagsAllMismatching) {
    const uint8_t tags[] = {0x04, 0x04, 0x04, 0x04};
    bool results[4] = {};

    asn1pp::arch_codec::batch_compare_tags(tags, 0x02, 4, results);

    for (size_t i = 0; i < 4; ++i) {
        EXPECT_FALSE(results[i]);
    }
}

// ============================================================================
// 6. Batch buffer tests
// ============================================================================

TEST(BatchBuffer, PushDrain) {
    asn1pp::batch_buffer<int, 4> buf;

    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_FALSE(buf.is_full());

    buf.push(10);
    buf.push(20);
    buf.push(30);
    buf.push(40);

    EXPECT_TRUE(buf.is_full());
    EXPECT_EQ(buf.size(), 4u);
    EXPECT_FALSE(buf.empty());

    EXPECT_EQ(buf[0], 10);
    EXPECT_EQ(buf[1], 20);
    EXPECT_EQ(buf[2], 30);
    EXPECT_EQ(buf[3], 40);

    auto drained = buf.drain();
    EXPECT_EQ(drained.size(), 4u);
    EXPECT_TRUE(buf.empty());
    EXPECT_FALSE(buf.is_full());

    EXPECT_EQ(drained[0], 10);
    EXPECT_EQ(drained[1], 20);
    EXPECT_EQ(drained[2], 30);
    EXPECT_EQ(drained[3], 40);
}

TEST(BatchBuffer, FlushAlias) {
    // flush() is alias for drain()
    asn1pp::batch_buffer<int, 2> buf;
    buf.push(1);
    buf.push(2);

    auto flushed = buf.flush();
    EXPECT_EQ(flushed.size(), 2u);
    EXPECT_EQ(flushed[0], 1);
    EXPECT_EQ(flushed[1], 2);
    EXPECT_TRUE(buf.empty());
}

TEST(BatchBuffer, DrainEmpty) {
    asn1pp::batch_buffer<int, 4> buf;
    auto drained = buf.drain();
    EXPECT_EQ(drained.size(), 0u);
    EXPECT_TRUE(buf.empty());
}

TEST(BatchBuffer, PushAfterDrainReusesCapacity) {
    asn1pp::batch_buffer<int, 4> buf;

    buf.push(1);
    buf.push(2);
    auto first = buf.drain();
    EXPECT_EQ(first.size(), 2u);

    // Push again after drain — should work
    buf.push(10);
    buf.push(20);
    buf.push(30);
    buf.push(40);

    EXPECT_TRUE(buf.is_full());

    auto second = buf.drain();
    EXPECT_EQ(second.size(), 4u);
    EXPECT_EQ(second[0], 10);
    EXPECT_EQ(second[1], 20);
    EXPECT_EQ(second[2], 30);
    EXPECT_EQ(second[3], 40);
}

TEST(BatchBuffer, Clear) {
    asn1pp::batch_buffer<int, 4> buf;
    buf.push(1);
    buf.push(2);
    buf.clear();

    EXPECT_TRUE(buf.empty());
    EXPECT_EQ(buf.size(), 0u);
}

TEST(BatchBuffer, SingleItemBuffer) {
    // N=1 buffer — should work for scalar pass-through
    asn1pp::batch_buffer<int, 1> buf;

    EXPECT_FALSE(buf.is_full());
    buf.push(42);
    EXPECT_TRUE(buf.is_full());

    auto drained = buf.drain();
    EXPECT_EQ(drained.size(), 1u);
    EXPECT_EQ(drained[0], 42);
    EXPECT_TRUE(buf.empty());
}

TEST(BatchBuffer, DataPointer) {
    asn1pp::batch_buffer<int, 4> buf;
    buf.push(1);
    buf.push(2);

    const int* p = buf.data();
    EXPECT_EQ(p[0], 1);
    EXPECT_EQ(p[1], 2);

    // Mutable data pointer
    int* mp = buf.data();
    mp[0] = 99;
    EXPECT_EQ(buf[0], 99);
}

TEST(BatchBuffer, MaxSizeConstant) {
    using Buf4 = asn1pp::batch_buffer<int, 4>;
    using Buf1 = asn1pp::batch_buffer<int, 1>;
    EXPECT_EQ(Buf4::max_size, 4u);
    EXPECT_EQ(Buf1::max_size, 1u);
}

// ============================================================================
// 7. BER decoder integration tests (SIMD batch path)
// ============================================================================

namespace {

asn1pp::buffer_view make_const_view(const std::vector<uint8_t>& vec) {
    return asn1pp::buffer_view(vec.data(), vec.size());
}

} // namespace

class BerSimdIntegrationTest : public ::testing::Test {
protected:
    asn1pp::ber::ber_decoder decoder_;
};

TEST_F(BerSimdIntegrationTest, DecodeSingleIntegerScalar) {
    // Single integer below batch size — scalar path
    std::vector<uint8_t> data = {0x02, 0x01, 0x7F};
    auto view = make_const_view(data);
    auto r = decoder_.decode_integer(view);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), 127);
}

TEST_F(BerSimdIntegrationTest, DecodeIntegersAndFlush) {
    // Decode up to 4 integers through the SIMD batching path,
    // then call flush_decode to drain remaining pending items.
    // The internal batching is transparent — decode_integer() may
    // return via scalar or SIMD path depending on batch_size.

    std::vector<uint8_t> data;
    data.insert(data.end(), {0x02, 0x01, 0x2A});       // INTEGER(42)
    data.insert(data.end(), {0x02, 0x01, 0xFF});       // INTEGER(-1)
    data.insert(data.end(), {0x02, 0x02, 0x01, 0x00}); // INTEGER(256)
    data.insert(data.end(), {0x02, 0x01, 0x7F});       // INTEGER(127)

    auto view = make_const_view(data);
    const size_t initial_size = view.size();

    // Decode all 4 integers — each call advances the buffer
    for (int i = 0; i < 4; ++i) {
        auto r = decoder_.decode_integer(view);
        ASSERT_TRUE(r.is_ok()) << "Failed at integer " << i;
        // Value correctness: each decoded value must be one of the expected set
        EXPECT_TRUE(r.value() == 42 || r.value() == -1 ||
                    r.value() == 256 || r.value() == 127)
            << "Unexpected decoded value: " << r.value();
    }

    // All 4 TLVs should now be consumed
    EXPECT_EQ(view.size(), 0u);
    EXPECT_NE(view.size(), initial_size);

    // Flush any remaining pending items (idempotent)
    auto flush_err = decoder_.flush_decode();
    EXPECT_EQ(flush_err, asn1pp::error_code::ok);
}

TEST_F(BerSimdIntegrationTest, FlushDecodeIdempotent) {
    // Calling flush_decode on empty buffer should return ok
    auto err = decoder_.flush_decode();
    EXPECT_EQ(err, asn1pp::error_code::ok);

    // Second call should also be ok
    err = decoder_.flush_decode();
    EXPECT_EQ(err, asn1pp::error_code::ok);
}

TEST_F(BerSimdIntegrationTest, DecodeAndAdvanceBuffer) {
    // Verify buffer_view advances after decode_integer
    std::vector<uint8_t> data = {
        0x02, 0x01, 0x01,   // INTEGER(1)
        0x02, 0x01, 0x02    // INTEGER(2)
    };
    auto view = make_const_view(data);

    auto r1 = decoder_.decode_integer(view);
    ASSERT_TRUE(r1.is_ok());
    EXPECT_EQ(r1.value(), 1);
    // view should have advanced past first TLV (3 bytes)
    EXPECT_EQ(view.size(), 3u);

    auto r2 = decoder_.decode_integer(view);
    ASSERT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.value(), 2);
    EXPECT_EQ(view.size(), 0u);
}

// ============================================================================
// 8. Additional edge-case tests
// ============================================================================

TEST(ArchCodec, BatchCompareTagsCountZero) {
    // Zero count should be a no-op
    bool result = false;
    asn1pp::arch_codec::batch_compare_tags(nullptr, 0x02, 0, &result);
    // No crash — success
}

TEST(ArchCodec, BatchDecodeSignExtend2ByteNegative) {
    // -256 in 2 bytes = 0xFF, 0x00
    const uint8_t buf[] = {0xFF, 0x00};
    const uint8_t* bufs[] = {buf};
    const size_t sizes[] = {2};
    int64_t out[1] = {};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_decode_integers(bufs, sizes, out, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(out[0], -256);
}

TEST(ArchCodec, BatchDecodeEightByteMaxUnsigned) {
    // Max uint64_t fits as int64_t positive? No, 0xFFFF... is -1 in signed.
    // 0x7FFFFFFFFFFFFFFF = INT64_MAX
    const uint8_t buf[] = {0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t* bufs[] = {buf};
    const size_t sizes[] = {8};
    int64_t out[1] = {};
    asn1pp::error_code errors[1] = {};

    asn1pp::arch_codec::batch_decode_integers(bufs, sizes, out, errors, 1);

    EXPECT_EQ(errors[0], asn1pp::error_code::ok);
    EXPECT_EQ(out[0], INT64_MAX);
}
