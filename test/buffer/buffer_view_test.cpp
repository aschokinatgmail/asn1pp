#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include <utility>

#include "buffer/buffer_view.hpp"

using namespace asn1pp;

// ============================================================================
// Static assertions (compile-time checks)
// ============================================================================

static_assert(std::is_trivially_copyable_v<buffer_view>,
              "buffer_view must be trivially copyable");

static_assert(sizeof(buffer_view) == 2 * sizeof(void*),
              "buffer_view must be exactly 2 pointers wide: data + size");

// ============================================================================
// Construction tests
// ============================================================================

TEST(BufferViewTest, ConstructFromUint8PtrAndSize) {
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    buffer_view view(data.data(), data.size());
    EXPECT_EQ(view.size(), 4);
    EXPECT_EQ(view.data(), data.data());
    EXPECT_FALSE(view.empty());
}

TEST(BufferViewTest, ConstructFromConstUint8PtrAndSize) {
    const uint8_t raw[] = {0xAA, 0xBB, 0xCC};
    buffer_view view(raw, 3);
    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view.data(), raw);
}

TEST(BufferViewTest, ConstructFromConstSpan) {
    const uint8_t raw[] = {0x10, 0x20, 0x30};
    std::span<const uint8_t> sp(raw);
    buffer_view view(sp);
    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view.data(), raw);
}

TEST(BufferViewTest, ConstructFromMutableSpan) {
    uint8_t raw[] = {0x40, 0x50, 0x60};
    std::span<uint8_t> sp(raw);
    buffer_view view(sp);
    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view.data(), raw);
}

TEST(BufferViewTest, ConstructFromVector) {
    std::vector<uint8_t> vec = {0x70, 0x80, 0x90};
    buffer_view view(vec);
    EXPECT_EQ(view.size(), 3);
    EXPECT_EQ(view.data(), vec.data());
}

TEST(BufferViewTest, DefaultConstructedViewIsEmpty) {
    buffer_view view;
    EXPECT_EQ(view.size(), 0);
    EXPECT_EQ(view.data(), nullptr);
    EXPECT_TRUE(view.empty());
}

TEST(BufferViewTest, ConstructWithNullPointerAndZeroSize) {
    buffer_view view(nullptr, 0);
    EXPECT_EQ(view.size(), 0);
    EXPECT_EQ(view.data(), nullptr);
    EXPECT_TRUE(view.empty());
}

// ============================================================================
// Element access tests
// ============================================================================

TEST(BufferViewTest, OperatorBracketAccess) {
    const uint8_t raw[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    buffer_view view(raw);
    EXPECT_EQ(view[0], 0x01);
    EXPECT_EQ(view[1], 0x02);
    EXPECT_EQ(view[2], 0x03);
    EXPECT_EQ(view[3], 0x04);
    EXPECT_EQ(view[4], 0x05);
}

TEST(BufferViewTest, FrontAndBack) {
    const uint8_t raw[] = {0xAA, 0xBB};
    buffer_view view(raw);
    // Simulate front/back via operator[]
    EXPECT_EQ(view[0], 0xAA);
    EXPECT_EQ(view[1], 0xBB);
}

// ============================================================================
// Subview / zero-copy slicing
// ============================================================================

TEST(BufferViewTest, SubviewCreatesZeroCopyView) {
    const uint8_t raw[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    buffer_view view(raw);
    buffer_view sub = view.subview(2, 4);
    EXPECT_EQ(sub.size(), 4);
    EXPECT_EQ(sub.data(), raw + 2);
    // Verify zero-copy: data pointer points into original
    EXPECT_EQ(sub[0], 0x02);
    EXPECT_EQ(sub[1], 0x03);
    EXPECT_EQ(sub[2], 0x04);
    EXPECT_EQ(sub[3], 0x05);
}

TEST(BufferViewTest, SubviewAtEndReturnsEmpty) {
    const uint8_t raw[] = {0x01, 0x02, 0x03};
    buffer_view view(raw);
    buffer_view sub = view.subview(3, 0);
    EXPECT_EQ(sub.size(), 0);
    EXPECT_TRUE(sub.empty());
}

TEST(BufferViewTest, SubviewEntireRange) {
    const uint8_t raw[] = {0x10, 0x20, 0x30};
    buffer_view view(raw);
    buffer_view sub = view.subview(0, 3);
    EXPECT_EQ(sub.size(), 3);
    EXPECT_EQ(sub.data(), raw);
}

// ============================================================================
// slice_at tests
// ============================================================================

TEST(BufferViewTest, SliceAtSplitsView) {
    const uint8_t raw[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    buffer_view view(raw);
    auto [left, right] = view.slice_at(3);
    EXPECT_EQ(left.size(), 3);
    EXPECT_EQ(left.data(), raw);
    EXPECT_EQ(right.size(), 3);
    EXPECT_EQ(right.data(), raw + 3);
    EXPECT_EQ(left[0], 0x00);
    EXPECT_EQ(right[0], 0x03);
}

TEST(BufferViewTest, SliceAtZero) {
    const uint8_t raw[] = {0x01, 0x02};
    buffer_view view(raw);
    auto [left, right] = view.slice_at(0);
    EXPECT_EQ(left.size(), 0);
    EXPECT_TRUE(left.empty());
    EXPECT_EQ(right.size(), 2);
    EXPECT_EQ(right.data(), raw);
}

TEST(BufferViewTest, SliceAtEnd) {
    const uint8_t raw[] = {0x01, 0x02};
    buffer_view view(raw);
    auto [left, right] = view.slice_at(2);
    EXPECT_EQ(left.size(), 2);
    EXPECT_EQ(left.data(), raw);
    EXPECT_EQ(right.size(), 0);
    EXPECT_TRUE(right.empty());
}

// ============================================================================
// Bit-level access tests (MSB-first)
// ============================================================================

TEST(BufferViewTest, BitAtMsbFirst) {
    // 0x80 = 1000 0000  -> bit 0 (MSB) is 1
    const uint8_t raw[] = {0x80};
    buffer_view view(raw);
    EXPECT_TRUE(view.bit_at(0));
    EXPECT_FALSE(view.bit_at(1));
}

TEST(BufferViewTest, BitAtMsbFirstByte0) {
    // 0xAA = 1010 1010
    // bit 0 = 1, bit 1 = 0, bit 2 = 1, bit 3 = 0, bit 4 = 1, bit 5 = 0, bit 6 = 1, bit 7 = 0
    const uint8_t raw[] = {0xAA};
    buffer_view view(raw);
    EXPECT_TRUE(view.bit_at(0));
    EXPECT_FALSE(view.bit_at(1));
    EXPECT_TRUE(view.bit_at(2));
    EXPECT_FALSE(view.bit_at(3));
    EXPECT_TRUE(view.bit_at(4));
    EXPECT_FALSE(view.bit_at(5));
    EXPECT_TRUE(view.bit_at(6));
    EXPECT_FALSE(view.bit_at(7));
}

TEST(BufferViewTest, BitAtCrossesByteBoundary) {
    // data[0] = 0x12 = 0001 0010
    // data[1] = 0x34 = 0011 0100
    // bit 4 = (0x12 >> 3) & 1 = 0
    // bit 5 = (0x12 >> 2) & 1 = 0
    // bit 6 = (0x12 >> 1) & 1 = 1
    // bit 7 = (0x12 >> 0) & 1 = 0
    // bit 8 = (0x34 >> 7) & 1 = 0 (MSB of byte 1)
    // bit 9 = (0x34 >> 6) & 1 = 0
    // bit 10 = (0x34 >> 5) & 1 = 1
    // bit 11 = (0x34 >> 4) & 1 = 1
    const uint8_t raw[] = {0x12, 0x34};
    buffer_view view(raw);
    EXPECT_FALSE(view.bit_at(4));
    EXPECT_FALSE(view.bit_at(5));
    EXPECT_TRUE(view.bit_at(6));
    EXPECT_FALSE(view.bit_at(7));
    EXPECT_FALSE(view.bit_at(8));
    EXPECT_FALSE(view.bit_at(9));
    EXPECT_TRUE(view.bit_at(10));
    EXPECT_TRUE(view.bit_at(11));
}

// ============================================================================
// bit_span tests (packed bit extraction)
// ============================================================================

TEST(BufferViewTest, BitSpanSingleByteFull) {
    // 0xAB = 1010 1011 -> bit_span(0, 8) = 0xAB
    const uint8_t raw[] = {0xAB};
    buffer_view view(raw);
    auto result = view.bit_span(0, 8);
    EXPECT_EQ(result.value, 0xAB);
}

TEST(BufferViewTest, BitSpanNibbleMsbFirst) {
    // 0xF0 = 1111 0000 -> bits 0-3 = 1111 = 0xF
    const uint8_t raw[] = {0xF0};
    buffer_view view(raw);
    auto result = view.bit_span(0, 4);
    EXPECT_EQ(result.value, 0xF);
}

TEST(BufferViewTest, BitSpanSingleBit) {
    // 0x80 = 1000 0000 -> bit_span(0, 1) = 1
    const uint8_t raw[] = {0x80};
    buffer_view view(raw);
    auto result = view.bit_span(0, 1);
    EXPECT_EQ(result.value, 1);
}

TEST(BufferViewTest, BitSpanZeroBit) {
    const uint8_t raw[] = {0xFF};
    buffer_view view(raw);
    auto result = view.bit_span(0, 0);
    EXPECT_EQ(result.value, 0);
}

TEST(BufferViewTest, BitSpanCrossesByteBoundary) {
    // data = {0x12, 0x34}
    // bit_span(4, 8): bits 4,5,6,7 of byte 0, bits 8,9,10,11 of byte 1
    // = 0, 0, 1, 0, 0, 0, 1, 1 = 0b00100011 = 0x23
    const uint8_t raw[] = {0x12, 0x34};
    buffer_view view(raw);
    auto result = view.bit_span(4, 8);
    EXPECT_EQ(result.value, 0x23);
}

TEST(BufferViewTest, BitSpanAtByteBoundary) {
    // data = {0x12, 0x34}
    // bit_span(8, 8) = all 8 bits of byte 1 = 0x34
    const uint8_t raw[] = {0x12, 0x34};
    buffer_view view(raw);
    auto result = view.bit_span(8, 8);
    EXPECT_EQ(result.value, 0x34);
}

TEST(BufferViewTest, BitSpanLargeValue) {
    // All-ones across multiple bytes, span of 32 bits
    const uint8_t raw[] = {0xFF, 0xFF, 0xFF, 0xFF};
    buffer_view view(raw);
    auto result = view.bit_span(0, 32);
    EXPECT_EQ(result.value, 0xFFFFFFFF);
}

// ============================================================================
// as_span() explicit conversion
// ============================================================================

TEST(BufferViewTest, AsSpanReturnsConstSpan) {
    const uint8_t raw[] = {0x01, 0x02, 0x03};
    buffer_view view(raw);
    std::span<const uint8_t> sp = view.as_span();
    EXPECT_EQ(sp.size(), 3);
    EXPECT_EQ(sp.data(), raw);
    EXPECT_EQ(sp[0], 0x01);
    EXPECT_EQ(sp[1], 0x02);
    EXPECT_EQ(sp[2], 0x03);
}

TEST(BufferViewTest, AsSpanOnEmptyView) {
    buffer_view view;
    std::span<const uint8_t> sp = view.as_span();
    EXPECT_EQ(sp.size(), 0);
    EXPECT_EQ(sp.data(), nullptr);
}

// ============================================================================
// Range-based for iteration
// ============================================================================

TEST(BufferViewTest, RangeForIteration) {
    const uint8_t raw[] = {0x0A, 0x0B, 0x0C};
    buffer_view view(raw);
    std::vector<uint8_t> collected;
    for (auto byte : view) {
        collected.push_back(byte);
    }
    ASSERT_EQ(collected.size(), 3);
    EXPECT_EQ(collected[0], 0x0A);
    EXPECT_EQ(collected[1], 0x0B);
    EXPECT_EQ(collected[2], 0x0C);
}

TEST(BufferViewTest, RangeForOnEmptyView) {
    buffer_view view;
    int count = 0;
    for ([[maybe_unused]] auto byte : view) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

// ============================================================================
// Copy semantics
// ============================================================================

TEST(BufferViewTest, CopyConstructedViewIsIdentical) {
    const uint8_t raw[] = {0x10, 0x20};
    buffer_view a(raw);
    buffer_view b = a;  // copy
    EXPECT_EQ(a.size(), b.size());
    EXPECT_EQ(a.data(), b.data());
    EXPECT_EQ(b[0], 0x10);
    EXPECT_EQ(b[1], 0x20);
}

TEST(BufferViewTest, CopyAssignViewIsIdentical) {
    const uint8_t raw[] = {0x30, 0x40};
    buffer_view a(raw);
    buffer_view b;
    b = a;
    EXPECT_EQ(a.size(), b.size());
    EXPECT_EQ(a.data(), b.data());
    EXPECT_EQ(b[0], 0x30);
    EXPECT_EQ(b[1], 0x40);
}

// ============================================================================
// Comparison tests
// ============================================================================

TEST(BufferViewTest, EqualViewsCompareEqual) {
    const uint8_t raw[] = {0x01, 0x02, 0x03};
    buffer_view a(raw);
    buffer_view b(raw);
    EXPECT_EQ(a, b);
}

TEST(BufferViewTest, ViewsPointingToDifferentMemoryCompareNotEqual) {
    const uint8_t raw1[] = {0x01, 0x02};
    const uint8_t raw2[] = {0x01, 0x02};
    buffer_view a(raw1);
    buffer_view b(raw2);
    EXPECT_NE(a, b);
}
