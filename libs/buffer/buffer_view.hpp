#pragma once

#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include <cassert>
#include <utility>

namespace asn1pp {

class buffer_view {
public:
    buffer_view() noexcept : data_(nullptr), size_(0) {}

    buffer_view(const uint8_t* data, size_t size) noexcept : data_(data), size_(size) {}

    explicit buffer_view(std::span<const uint8_t> sp) noexcept : data_(sp.data()), size_(sp.size()) {}

    explicit buffer_view(std::span<uint8_t> sp) noexcept : data_(sp.data()), size_(sp.size()) {}

    explicit buffer_view(std::vector<uint8_t>& vec) noexcept : data_(vec.data()), size_(vec.size()) {}

    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] const uint8_t* data() const noexcept { return data_; }

    [[nodiscard]] const uint8_t& operator[](size_t idx) const noexcept {
        assert(idx < size_);
        return data_[idx];
    }

    [[nodiscard]] buffer_view subview(size_t offset, size_t length) const noexcept {
        assert(offset + length <= size_);
        return buffer_view(data_ + offset, length);
    }

    [[nodiscard]] std::pair<buffer_view, buffer_view> slice_at(size_t offset) const noexcept {
        assert(offset <= size_);
        return {subview(0, offset), subview(offset, size_ - offset)};
    }

    [[nodiscard]] bool bit_at(size_t bit_offset) const noexcept {
        const size_t byte_idx = bit_offset / 8;
        const size_t bit_idx = bit_offset % 8;
        assert(byte_idx < size_);
        // MSB-first: bit 0 of a byte is its most significant bit (bit 7 in LSB notation)
        return (data_[byte_idx] >> (7 - bit_idx)) & 1U;
    }

    struct bit_span_result {
        uint64_t value;
    };

    [[nodiscard]] bit_span_result bit_span(size_t bit_offset, size_t bit_count) const noexcept {
        assert(bit_count <= 64);
        assert(bit_offset + bit_count <= size_ * 8);
        uint64_t result = 0;
        for (size_t i = 0; i < bit_count; ++i) {
            result = (result << 1) | (bit_at(bit_offset + i) ? 1ULL : 0ULL);
        }
        return bit_span_result{result};
    }

    [[nodiscard]] std::span<const uint8_t> as_span() const noexcept {
        return std::span<const uint8_t>{data_, size_};
    }

    [[nodiscard]] const uint8_t* begin() const noexcept { return data_; }
    [[nodiscard]] const uint8_t* end() const noexcept { return data_ + size_; }

    [[nodiscard]] friend bool operator==(const buffer_view& a, const buffer_view& b) noexcept {
        return a.data_ == b.data_ && a.size_ == b.size_;
    }

    [[nodiscard]] friend bool operator!=(const buffer_view& a, const buffer_view& b) noexcept {
        return !(a == b);
    }

private:
    const uint8_t* data_;
    size_t size_;
};

} // namespace asn1pp
