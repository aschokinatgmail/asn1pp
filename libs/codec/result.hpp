#pragma once

#include <cstdint>
#include <cassert>
#include <new>
#include <utility>
#include <cstddef>

namespace asn1pp {

enum class error_code : uint8_t {
    ok                  = 0,
    buffer_overflow     = 1,
    buffer_underflow    = 2,
    invalid_tag         = 3,
    invalid_length      = 4,
    constraint_violation = 5,
    parse_error         = 6,
    unexpected_extension = 7,
    value_out_of_range  = 8,
    encoding_error      = 9,
};

namespace detail {

template<typename T>
struct result_storage {
    alignas(T) unsigned char data[sizeof(T)];
    error_code error;
    bool is_ok;

    constexpr result_storage() noexcept : data{}, error(error_code::ok), is_ok(true) {}

    template<typename... Args>
    constexpr void construct_ok(Args&&... args) {
        new (data) T(std::forward<Args>(args)...);
        error = error_code::ok;
        is_ok = true;
    }

    constexpr void construct_err(error_code e) {
        error = e;
        is_ok = false;
    }

    constexpr T& value() noexcept {
        return *reinterpret_cast<T*>(data);
    }

    constexpr const T& value() const noexcept {
        return *reinterpret_cast<const T*>(data);
    }
};

template<>
struct result_storage<void> {
    error_code error;
    bool is_ok;

    constexpr result_storage() noexcept : error(error_code::ok), is_ok(true) {}

    constexpr void construct_ok() noexcept {
        error = error_code::ok;
        is_ok = true;
    }

    constexpr void construct_err(error_code e) noexcept {
        error = e;
        is_ok = false;
    }
};

} // namespace detail

template<typename T>
class result {
private:
    detail::result_storage<T> storage_;

public:
    using value_type = T;

    constexpr result() noexcept = default;

    static constexpr result<T> ok(T val) noexcept {
        result<T> r;
        r.storage_.construct_ok(std::move(val));
        return r;
    }

    static constexpr result<T> err(error_code e) noexcept {
        result<T> r;
        r.storage_.construct_err(e);
        return r;
    }

    constexpr bool is_ok() const noexcept {
        return storage_.is_ok;
    }

    constexpr bool is_err() const noexcept {
        return !storage_.is_ok;
    }

    constexpr T& value() & noexcept {
        assert(storage_.is_ok && "result::value() called on error state");
        return storage_.value();
    }

    constexpr const T& value() const & noexcept {
        assert(storage_.is_ok && "result::value() called on error state");
        return storage_.value();
    }

    constexpr T&& value() && noexcept {
        assert(storage_.is_ok && "result::value() called on error state");
        return std::move(storage_.value());
    }

    constexpr error_code error() const noexcept {
        return storage_.error;
    }

    template<typename F>
    constexpr auto map(F&& f) noexcept -> result<decltype(f(std::declval<T>()))> {
        using U = decltype(f(std::declval<T>()));
        if (storage_.is_ok) {
            return result<U>::ok(f(std::move(storage_.value())));
        }
        return result<U>::err(storage_.error);
    }

    template<typename F>
    constexpr auto and_then(F&& f) noexcept -> decltype(f(std::declval<T>())) {
        if (storage_.is_ok) {
            return f(std::move(storage_.value()));
        }
        using ResultType = decltype(f(std::declval<T>()));
        return ResultType::err(storage_.error);
    }
};

template<>
class result<void> {
private:
    detail::result_storage<void> storage_;

public:
    using value_type = void;

    constexpr result() noexcept = default;

    static constexpr result<void> ok() noexcept {
        result<void> r;
        r.storage_.construct_ok();
        return r;
    }

    static constexpr result<void> err(error_code e) noexcept {
        result<void> r;
        r.storage_.construct_err(e);
        return r;
    }

    constexpr bool is_ok() const noexcept {
        return storage_.is_ok;
    }

    constexpr bool is_err() const noexcept {
        return !storage_.is_ok;
    }

    constexpr error_code error() const noexcept {
        return storage_.error;
    }

    template<typename F>
    constexpr auto map(F&& f) noexcept -> result<decltype(f())> {
        using U = decltype(f());
        if (storage_.is_ok) {
            return result<U>::ok(f());
        }
        return result<U>::err(storage_.error);
    }

    template<typename F>
    constexpr auto and_then(F&& f) noexcept -> decltype(f()) {
        if (storage_.is_ok) {
            return f();
        }
        using ResultType = decltype(f());
        return ResultType::err(storage_.error);
    }
};

} // namespace asn1pp