#pragma once

#include <concepts>
#include <type_traits>
#include "buffer/buffer_view.hpp"
#include "codec/result.hpp"
#include "codec/traits.hpp"

namespace asn1pp {

template<typename E, typename T>
concept encoder_for = requires(E e, const T& val, buffer_view buf) {
    { e.encode(val, buf) } -> std::same_as<result<void>>;
};

template<typename D, typename T>
concept decoder_for = requires(D d, buffer_view buf) {
    { d.decode(buf) } -> std::same_as<result<T>>;
};

template<typename C, typename T>
concept codec_for = encoder_for<C, T> && decoder_for<C, T>;

template<typename C>
concept stateless_codec = std::is_empty_v<C>;

template<typename Derived>
class encoder_base {
public:
    template<typename... Fields>
    result<void> encode_fields(buffer_view& buf, const Fields&... fields) {
        (void)buf;
        ((void)fields, ...);
        return result<void>::ok();
    }

    template<typename FieldType>
    result<void> encode_tagged(const tag& t, const FieldType& val, buffer_view& buf) {
        (void)t;
        (void)val;
        (void)buf;
        return result<void>::ok();
    }

protected:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

template<typename Derived>
class decoder_base {
public:
    template<typename FieldType>
    result<FieldType> decode_tagged(const tag& expected_tag, buffer_view& buf) {
        (void)expected_tag;
        (void)buf;
        return result<FieldType>::err(error_code::parse_error);
    }

protected:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
};

} // namespace asn1pp
