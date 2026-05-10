#pragma once

#include <type_traits>
#include "codec/traits.hpp"

namespace asn1pp {

/// IMPLICIT tagged wrapper: the outer tag replaces the original universal tag,
/// but the value encoding of the underlying type is preserved.
///
/// Example:
///   [APPLICATION 1] IMPLICIT INTEGER
///   Encodes as: Application [1] + INTEGER two's-complement encoding
///
/// For PER: IMPLICIT tagging has no effect (tags are not encoded in PER).
template<typename T>
struct implicit_tagged {
    using value_type = T;
    T value{};
    static constexpr tag override_tag_val = tag_for_type_v<T>;

    implicit_tagged() = default;
    explicit implicit_tagged(T v) : value(std::move(v)) {}

    bool operator==(const implicit_tagged&) const = default;
};

/// EXPLICIT tagged wrapper: the outer tag wraps a complete inner TLV of the
/// underlying type.
///
/// Example:
///   [3] EXPLICIT SEQUENCE { a INTEGER }
///   Encodes as: Context [3] constructed + length + SEQUENCE TLV
///
/// For PER: EXPLICIT tagging has no effect (the outer TLV is not encoded in PER).
template<typename T>
struct explicit_tagged {
    using value_type = T;
    T value{};
    static constexpr tag outer_tag_val = tag_for_type_v<T>;

    explicit_tagged() = default;
    explicit explicit_tagged(T v) : value(std::move(v)) {}

    bool operator==(const explicit_tagged&) const = default;
};

// ── Primitive type trait specializations (not generated, added here) ─

template<> struct is_type_primitive<int64_t> : std::true_type {};
template<> struct is_type_primitive<uint64_t> : std::true_type {};
template<> struct is_type_primitive<bool> : std::true_type {};
template<> struct is_type_primitive<double> : std::true_type {};
template<> struct is_type_primitive<std::string> : std::true_type {};
template<> struct is_type_primitive<std::vector<uint8_t>> : std::true_type {};
template<> struct is_type_primitive<std::vector<uint32_t>> : std::true_type {};
template<> struct is_type_primitive<std::monostate> : std::true_type {};

template<> struct is_type_constructed<std::pair<std::vector<uint8_t>, size_t>>
    : std::true_type {};

// ── Trait specializations: IMPLICIT tagged ───────────────────────────

template<typename T>
struct tag_for_type<implicit_tagged<T>> {
    static constexpr tag value = tag_for_type<T>::value;
};

template<typename T>
struct is_type_constructed<implicit_tagged<T>> : is_type_constructed<T> {};

template<typename T>
struct is_type_primitive<implicit_tagged<T>> : is_type_primitive<T> {};

// ── Trait specializations: EXPLICIT tagged ───────────────────────────

template<typename T>
struct tag_for_type<explicit_tagged<T>> {
    static constexpr tag value = tag_for_type<T>::value;
};

template<typename T>
struct is_type_constructed<explicit_tagged<T>> : is_type_constructed<T> {};

template<typename T>
struct is_type_primitive<explicit_tagged<T>> : is_type_primitive<T> {};

// ── Explicit outer tag for EXPLICIT tagged types ─────────────────────

/// Primary template: most types have no explicit outer tag.
template<typename T>
struct asn1_explicit_outer_tag {
    static constexpr bool has_outer = false;
    static constexpr tag outer{};
};

template<typename T>
inline constexpr bool has_explicit_outer_tag_v = asn1_explicit_outer_tag<T>::has_outer;

template<typename T>
struct asn1_explicit_outer_tag<explicit_tagged<T>> {
    static constexpr bool has_outer = true;
    static constexpr tag outer = explicit_tagged<T>::outer_tag_val;
};

}  // namespace asn1pp
