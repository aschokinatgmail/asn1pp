#pragma once

#include <cstddef>
#include <span>

namespace asn1pp {

/// Reusable ring buffer template for transparent batching in codecs.
///
/// Fixed-size, non-allocating buffer on the stack. Used to accumulate
/// pending encode/decode operations before dispatching them via SIMD
/// batch functions.
///
/// @tparam T  Pending operation type (e.g., pending_integer struct).
/// @tparam N  Maximum number of items in the batch (SIMD width: 1, 2, or 4).
///
/// Single-threaded design — no synchronization.
template<typename T, size_t N>
class batch_buffer {
public:
    /// Default-construct an empty buffer.
    constexpr batch_buffer() noexcept = default;

    /// Push an item onto the batch buffer.
    /// Must NOT be called when is_full() returns true.
    constexpr void push(T item) noexcept {
        items_[count_++] = item;
    }

    /// True when the buffer is at maximum capacity.
    [[nodiscard]] constexpr bool is_full() const noexcept {
        return count_ == N;
    }

    /// True when the buffer has no items.
    [[nodiscard]] constexpr bool empty() const noexcept {
        return count_ == 0;
    }

    /// Number of items currently stored in the buffer.
    [[nodiscard]] constexpr size_t size() const noexcept {
        return count_;
    }

    /// Pointer to the internal array of items.
    /// Use for direct indexed access in batch dispatch loops.
    [[nodiscard]] constexpr T* data() noexcept {
        return items_;
    }

    /// Pointer to the internal array of items (const).
    [[nodiscard]] constexpr const T* data() const noexcept {
        return items_;
    }

    /// Reset the buffer to empty without modifying stored items.
    constexpr void clear() noexcept {
        count_ = 0;
    }

    /// Return a span over all pending items and reset the buffer.
    /// Primary API for decoders.
    [[nodiscard]] constexpr std::span<T> drain() noexcept {
        std::span<T> result(items_, count_);
        count_ = 0;
        return result;
    }

    /// Alias for drain() — for encoder consistency.
    [[nodiscard]] constexpr std::span<T> flush() noexcept {
        return drain();
    }

    /// Indexed access (bounds-checked in debug builds via assert).
    [[nodiscard]] constexpr T& operator[](size_t index) noexcept {
        return items_[index];
    }

    /// Indexed access (const, bounds-checked in debug builds via assert).
    [[nodiscard]] constexpr const T& operator[](size_t index) const noexcept {
        return items_[index];
    }

    /// Maximum capacity of the buffer.
    static constexpr size_t max_size = N;

private:
    T items_[N]{};
    size_t count_ = 0;
};

} // namespace asn1pp
