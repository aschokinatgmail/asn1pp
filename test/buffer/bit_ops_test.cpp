#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <array>
#include <cstring>

#include "buffer/buffer_view.hpp"
#include "buffer/bit_ops.hpp"

using namespace asn1pp;

static_assert(std::is_trivially_copyable_v<buffer_view>,
              "buffer_view must be trivially copyable");

TEST(BitOpsTest, WriteReadBits1) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 1, 1);
    EXPECT_EQ(bit_offset, 1);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 1);
    EXPECT_EQ(val, 1);
}

TEST(BitOpsTest, WriteReadBits8) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0xAB, 8);
    EXPECT_EQ(bit_offset, 8);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 8);
    EXPECT_EQ(val, 0xAB);
}

TEST(BitOpsTest, WriteReadBits12) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0xABC, 12);
    EXPECT_EQ(bit_offset, 12);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 12);
    EXPECT_EQ(val, 0xABC);
}

TEST(BitOpsTest, WriteReadBits16) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0x1234, 16);
    EXPECT_EQ(bit_offset, 16);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 16);
    EXPECT_EQ(val, 0x1234);
}

TEST(BitOpsTest, WriteReadBits32) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0x12345678, 32);
    EXPECT_EQ(bit_offset, 32);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 32);
    EXPECT_EQ(val, 0x12345678);
}

TEST(BitOpsTest, WriteReadBits63) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0x7FFFFFFFFFFFFFFF, 63);
    EXPECT_EQ(bit_offset, 63);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 63);
    EXPECT_EQ(val, 0x7FFFFFFFFFFFFFFF);
}

TEST(BitOpsTest, WriteReadBitsSpanningBytes) {
    uint8_t buf[16] = {0};
    uint8_t src[] = {0xAB};
    size_t byte_offset = 0;

    write_octets(buf, byte_offset, src, 1);
    EXPECT_EQ(byte_offset, 1);
    EXPECT_EQ(buf[0], 0xAB);

    byte_offset = 0;
    uint8_t dst[16] = {0};
    read_octets(buf, byte_offset, dst, 1);
    EXPECT_EQ(byte_offset, 1);
    EXPECT_EQ(dst[0], 0xAB);
}

TEST(BitOpsTest, WriteReadOctets4) {
    uint8_t buf[16] = {0};
    uint8_t src[] = {0x12, 0x34, 0x56, 0x78};
    size_t byte_offset = 0;

    write_octets(buf, byte_offset, src, 4);
    EXPECT_EQ(byte_offset, 4);
    EXPECT_EQ(buf[0], 0x12);
    EXPECT_EQ(buf[1], 0x34);
    EXPECT_EQ(buf[2], 0x56);
    EXPECT_EQ(buf[3], 0x78);

    byte_offset = 0;
    uint8_t dst[16] = {0};
    read_octets(buf, byte_offset, dst, 4);
    EXPECT_EQ(byte_offset, 4);
    EXPECT_EQ(dst[0], 0x12);
    EXPECT_EQ(dst[1], 0x34);
    EXPECT_EQ(dst[2], 0x56);
    EXPECT_EQ(dst[3], 0x78);
}

TEST(BitOpsTest, WriteReadOctets256) {
    std::vector<uint8_t> src(256);
    for (size_t i = 0; i < 256; ++i) {
        src[i] = static_cast<uint8_t>(i);
    }

    std::vector<uint8_t> buf(256, 0);
    size_t byte_offset = 0;

    write_octets(buf.data(), byte_offset, src.data(), 256);
    EXPECT_EQ(byte_offset, 256);

    std::vector<uint8_t> dst(256, 0);
    byte_offset = 0;
    read_octets(buf.data(), byte_offset, dst.data(), 256);
    EXPECT_EQ(byte_offset, 256);

    EXPECT_EQ(src, dst);
}

TEST(BitOpsTest, AlignToOctetWhenAligned) {
    size_t bit_offset = 8;
    align_to_octet(bit_offset);
    EXPECT_EQ(bit_offset, 8);
}

TEST(BitOpsTest, AlignToOctetWhenNotAligned1) {
    size_t bit_offset = 1;
    align_to_octet(bit_offset);
    EXPECT_EQ(bit_offset, 8);
}

TEST(BitOpsTest, AlignToOctetWhenNotAligned7) {
    size_t bit_offset = 7;
    align_to_octet(bit_offset);
    EXPECT_EQ(bit_offset, 8);
}

TEST(BitOpsTest, AlignToOctetAfterPartialBitWrite) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0x5, 3);
    EXPECT_EQ(bit_offset, 3);

    align_to_octet(bit_offset);
    EXPECT_EQ(bit_offset, 8);

    write_bits(buf, bit_offset, 0xAB, 8);
    EXPECT_EQ(bit_offset, 16);

    bit_offset = 0;
    uint64_t val1 = read_bits(buf, bit_offset, 3);
    EXPECT_EQ(val1, 0x5);

    bit_offset = 8;
    uint64_t val2 = read_bits(buf, bit_offset, 8);
    EXPECT_EQ(val2, 0xAB);
}

TEST(BitOpsTest, RemainingBitsEmpty) {
    uint8_t buf[8] = {0};
    size_t buf_size = 8;
    size_t bit_offset = 0;

    size_t remaining = remaining_bits(buf, buf_size, bit_offset);
    EXPECT_EQ(remaining, 64);
}

TEST(BitOpsTest, RemainingBitsPartial) {
    uint8_t buf[8] = {0};
    size_t buf_size = 8;
    size_t bit_offset = 16;

    size_t remaining = remaining_bits(buf, buf_size, bit_offset);
    EXPECT_EQ(remaining, 48);
}

TEST(BitOpsTest, RemainingOctetsEmpty) {
    uint8_t buf[8] = {0};
    size_t buf_size = 8;
    size_t byte_offset = 0;

    size_t remaining = remaining_octets(buf, buf_size, byte_offset);
    EXPECT_EQ(remaining, 8);
}

TEST(BitOpsTest, RemainingOctetsPartial) {
    uint8_t buf[8] = {0};
    size_t buf_size = 8;
    size_t byte_offset = 3;

    size_t remaining = remaining_octets(buf, buf_size, byte_offset);
    EXPECT_EQ(remaining, 5);
}

TEST(BitOpsTest, MixedWriteBitsThenOctets) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0xA, 4);
    EXPECT_EQ(bit_offset, 4);

    align_to_octet(bit_offset);
    EXPECT_EQ(bit_offset, 8);

    size_t byte_offset = bit_offset / 8;
    uint8_t src[] = {0x12, 0x34, 0x56, 0x78};
    write_octets(buf, byte_offset, src, 4);
    EXPECT_EQ(byte_offset, 5);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 4);
    EXPECT_EQ(val, 0xA);

    byte_offset = 1;
    uint8_t dst[4] = {0};
    read_octets(buf, byte_offset, dst, 4);
    EXPECT_EQ(dst[0], 0x12);
    EXPECT_EQ(dst[1], 0x34);
    EXPECT_EQ(dst[2], 0x56);
    EXPECT_EQ(dst[3], 0x78);
}

TEST(BitOpsTest, MixedWriteOctetsThenBits) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;
    size_t byte_offset = 0;

    uint8_t src[] = {0x12, 0x34, 0x56, 0x78};
    write_octets(buf, byte_offset, src, 4);
    EXPECT_EQ(byte_offset, 4);

    bit_offset = 32;

    write_bits(buf, bit_offset, 0xABC, 12);
    EXPECT_EQ(bit_offset, 44);

    byte_offset = 0;
    uint8_t dst[4] = {0};
    read_octets(buf, byte_offset, dst, 4);
    EXPECT_EQ(dst[0], 0x12);
    EXPECT_EQ(dst[1], 0x34);
    EXPECT_EQ(dst[2], 0x56);
    EXPECT_EQ(dst[3], 0x78);

    bit_offset = 32;
    uint64_t val = read_bits(buf, bit_offset, 12);
    EXPECT_EQ(val, 0xABC);
}


TEST(BitOpsTest, MSBFirstBitOrder) {
    uint8_t buf[2] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0x80, 8);
    EXPECT_EQ(bit_offset, 8);

    EXPECT_EQ(buf[0], 0x80);
    EXPECT_EQ(buf[1], 0x00);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 8);
    EXPECT_EQ(val, 0x80);
}

TEST(BitOpsTest, MSBFirstAcrossByteBoundary) {
    uint8_t buf[2] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0x1234, 16);
    EXPECT_EQ(bit_offset, 16);

    EXPECT_EQ(buf[0], 0x12);
    EXPECT_EQ(buf[1], 0x34);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 16);
    EXPECT_EQ(val, 0x1234);
}


TEST(BitOpsTest, WriteZeroThenReadNonZeroBits) {
    uint8_t buf[16] = {0xFF, 0xFF};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0, 64);
    EXPECT_EQ(bit_offset, 64);

    EXPECT_EQ(buf[0], 0x00);
    EXPECT_EQ(buf[1], 0x00);
    EXPECT_EQ(buf[7], 0x00);
}

TEST(BitOpsTest, WriteAllOnesReadBack) {
    uint8_t buf[16] = {0};
    size_t bit_offset = 0;

    write_bits(buf, bit_offset, 0xFFFF, 16);
    EXPECT_EQ(bit_offset, 16);

    EXPECT_EQ(buf[0], 0xFF);
    EXPECT_EQ(buf[1], 0xFF);

    bit_offset = 0;
    uint64_t val = read_bits(buf, bit_offset, 16);
    EXPECT_EQ(val, 0xFFFF);
}
