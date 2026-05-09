#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <concepts>
#include "buffer/buffer_view.hpp"

namespace asn1pp {

enum class universal_tag : uint32_t {
    end_of_content    = 0,
    boolean           = 1,
    integer           = 2,
    bit_string        = 3,
    octet_string      = 4,
    null              = 5,
    object_identifier = 6,
    object_descriptor = 7,
    external          = 8,
    real              = 9,
    enumerated        = 10,
    embedded_pdv      = 11,
    utf8_string       = 12,
    relative_oid      = 13,
    sequence          = 16,
    set               = 17,
    numeric_string    = 18,
    printable_string  = 19,
    teletex_string    = 20,
    videotex_string   = 21,
    ia5_string        = 22,
    utc_time          = 23,
    generalized_time  = 24,
    graphic_string    = 25,
    visible_string    = 26,
    general_string    = 27,
    universal_string  = 28,
    character_string  = 29,
    bmp_string        = 30,
    date              = 31,
    time_of_day       = 32,
    datetime          = 33,
    duration          = 34,
    rel_uri           = 35,
    rel_uris          = 36,
};

enum class tag_class : uint8_t {
    universal        = 0,
    application      = 1,
    context_specific = 2,
    private_class    = 3,
};

struct tag {
    tag_class cls;
    bool constructed;
    uint32_t number;

    constexpr bool operator==(const tag&) const = default;
};

constexpr tag make_universal(universal_tag t, bool constructed = false) {
    return {tag_class::universal, constructed, static_cast<uint32_t>(t)};
}

constexpr tag make_context_specific(uint32_t number, bool constructed = false) {
    return {tag_class::context_specific, constructed, number};
}

constexpr bool is_primitive(universal_tag t) {
    using UT = universal_tag;
    switch (t) {
        case UT::boolean:
        case UT::integer:
        case UT::bit_string:
        case UT::octet_string:
        case UT::null:
        case UT::object_identifier:
        case UT::object_descriptor:
        case UT::real:
        case UT::enumerated:
        case UT::utf8_string:
        case UT::relative_oid:
        case UT::numeric_string:
        case UT::printable_string:
        case UT::teletex_string:
        case UT::videotex_string:
        case UT::ia5_string:
        case UT::utc_time:
        case UT::generalized_time:
        case UT::graphic_string:
        case UT::visible_string:
        case UT::general_string:
        case UT::universal_string:
        case UT::character_string:
        case UT::bmp_string:
        case UT::date:
        case UT::time_of_day:
        case UT::datetime:
            return true;
        default:
            return false;
    }
}

constexpr bool is_constructed(universal_tag t) {
    using UT = universal_tag;
    switch (t) {
        case UT::sequence:
        case UT::set:
        case UT::duration:
        case UT::rel_uris:
            return true;
        default:
            return false;
    }
}

constexpr const char* universal_tag_name(universal_tag t) {
    using UT = universal_tag;
    switch (t) {
        case UT::end_of_content:    return "END-OF-CONTENT";
        case UT::boolean:           return "BOOLEAN";
        case UT::integer:           return "INTEGER";
        case UT::bit_string:        return "BIT STRING";
        case UT::octet_string:      return "OCTET STRING";
        case UT::null:              return "NULL";
        case UT::object_identifier: return "OBJECT IDENTIFIER";
        case UT::object_descriptor: return "ObjectDescriptor";
        case UT::external:          return "EXTERNAL";
        case UT::real:              return "REAL";
        case UT::enumerated:        return "ENUMERATED";
        case UT::embedded_pdv:      return "EMBEDDED PDV";
        case UT::utf8_string:       return "UTF8String";
        case UT::relative_oid:      return "RELATIVE-OID";
        case UT::sequence:          return "SEQUENCE";
        case UT::set:               return "SET";
        case UT::numeric_string:    return "NumericString";
        case UT::printable_string:  return "PrintableString";
        case UT::teletex_string:    return "TeletexString";
        case UT::videotex_string:   return "VideotexString";
        case UT::ia5_string:        return "IA5String";
        case UT::utc_time:          return "UTCTime";
        case UT::generalized_time:  return "GeneralizedTime";
        case UT::graphic_string:    return "GraphicString";
        case UT::visible_string:    return "VisibleString";
        case UT::general_string:    return "GeneralString";
        case UT::universal_string:  return "UniversalString";
        case UT::character_string:  return "CharacterString";
        case UT::bmp_string:        return "BMPString";
        case UT::date:             return "DATE";
        case UT::time_of_day:      return "TIME-OF-DAY";
        case UT::datetime:         return "DATE-TIME";
        case UT::duration:          return "DURATION";
        case UT::rel_uri:           return "RELATIVE-URI";
        case UT::rel_uris:          return "RELATIVE-URIS";
    }
    return "UNKNOWN";
}

template<typename T>
struct asn1_tag {
    static constexpr auto value = universal_tag::null;
    static constexpr bool is_specialized = false;
};

template<typename T>
inline constexpr auto asn1_tag_v = asn1_tag<T>::value;

template<typename T>
concept has_tag = requires {
    { asn1_tag<T>::is_specialized } -> std::convertible_to<bool>;
    { asn1_tag<T>::value } -> std::convertible_to<universal_tag>;
} && asn1_tag<T>::is_specialized;

template<typename T>
inline constexpr bool has_tag_v = has_tag<T>;

template<typename T>
struct is_sequence : std::false_type {};

template<typename T>
inline constexpr bool is_sequence_v = is_sequence<T>::value;

template<typename T>
struct is_choice : std::false_type {};

template<typename T>
inline constexpr bool is_choice_v = is_choice<T>::value;

template<typename T>
struct is_set : std::false_type {};

template<typename T>
inline constexpr bool is_set_v = is_set<T>::value;

template<typename T>
concept asn1_type = requires { typename T::asn1_type_tag; } || has_tag_v<T>;

template<typename T>
struct tag_for_type;

template<typename T>
inline constexpr tag tag_for_type_v = tag_for_type<T>::value;

template<>
struct tag_for_type<int64_t> {
    static constexpr tag value = make_universal(universal_tag::integer);
};

template<>
struct tag_for_type<uint64_t> {
    static constexpr tag value = make_universal(universal_tag::integer);
};

template<>
struct tag_for_type<bool> {
    static constexpr tag value = make_universal(universal_tag::boolean);
};

template<>
struct tag_for_type<std::string> {
    static constexpr tag value = make_universal(universal_tag::octet_string);
};

template<>
struct tag_for_type<std::string_view> {
    static constexpr tag value = make_universal(universal_tag::utf8_string);
};

template<typename T>
struct is_type_constructed : std::false_type {};

template<typename T>
inline constexpr bool is_type_constructed_v = is_type_constructed<T>::value;

template<typename T>
struct is_type_primitive : std::false_type {};

template<typename T>
inline constexpr bool is_type_primitive_v = is_type_primitive<T>::value;

// gen type trait specializations are defined in src/gen/ast.hpp
// (after all gen types are fully defined) to avoid incomplete-type errors.

}  // namespace asn1pp