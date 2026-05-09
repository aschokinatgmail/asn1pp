#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <optional>
#include <memory>

#include "../../libs/codec/traits.hpp"

namespace asn1pp::gen {

enum class tag_default { explicit_tag, implicit_tag, automatic_tag };

struct source_location {
    std::string_view file;
    size_t line = 0;
    size_t column = 0;
};

struct integer_type {};
struct boolean_type {};
struct null_type {};
struct real_type {};
struct octet_string_type {};
struct object_identifier_type {};
struct relative_oid_type {};
struct any_type {};

struct enumeration_item {
    std::string name;
    std::optional<int64_t> value;
};

struct named_bit {
    std::string name;
    std::optional<int64_t> position;
};

struct enumerated_type {
    std::vector<enumeration_item> values;
    bool has_extension = false;
};

struct bit_string_type {
    std::vector<named_bit> named_bits;
    bool has_extension = false;
};

struct type_ref;
struct sequence_type;
struct set_type;
struct choice_type;
struct sequence_of_type;
struct set_of_type;
struct tagged_type;
struct constrained_type;

struct type_ref {
    using variant_type = std::variant<
        integer_type,
        boolean_type,
        null_type,
        real_type,
        octet_string_type,
        object_identifier_type,
        relative_oid_type,
        any_type,
        std::string,
        std::unique_ptr<sequence_type>,
        std::unique_ptr<set_type>,
        std::unique_ptr<choice_type>,
        enumerated_type,
        bit_string_type,
        std::unique_ptr<sequence_of_type>,
        std::unique_ptr<set_of_type>,
        std::unique_ptr<tagged_type>,
        std::unique_ptr<constrained_type>
    >;
    variant_type content;

    type_ref() = default;
    ~type_ref() = default;
    type_ref(type_ref&&) noexcept = default;
    type_ref& operator=(type_ref&&) noexcept = default;
    type_ref(const type_ref&) = delete;
    type_ref& operator=(const type_ref&) = delete;

    template<typename T>
    [[nodiscard]] bool holds_alternative() const noexcept {
        return std::holds_alternative<T>(content);
    }

    template<typename T>
    [[nodiscard]] const T& get() const {
        return std::get<T>(content);
    }

    template<typename T>
    [[nodiscard]] bool holds_ptr() const noexcept {
        return std::holds_alternative<std::unique_ptr<T>>(content);
    }

    template<typename T>
    [[nodiscard]] const T& get_ptr() const {
        return *std::get<std::unique_ptr<T>>(content);
    }
};

struct component_type {
    std::string name;
    std::unique_ptr<type_ref> type;
    bool optional = false;
    std::optional<std::string> default_value;
};

struct sequence_type {
    std::vector<component_type> components;
    bool has_extension = false;
};

struct set_type {
    std::vector<component_type> components;
    bool has_extension = false;
};

struct choice_alternative {
    std::string name;
    std::unique_ptr<type_ref> type;
};

struct choice_type {
    std::vector<choice_alternative> alternatives;
    bool has_extension = false;
};

struct sequence_of_type {
    std::unique_ptr<type_ref> element_type;
};

struct set_of_type {
    std::unique_ptr<type_ref> element_type;
};

struct tagged_type {
    asn1pp::tag tag_value;
    bool implicit = true;
    std::unique_ptr<type_ref> underlying_type;
};

struct selection_type {
    std::unique_ptr<type_ref> selected_type;
    std::string field_name;
};

struct value_range_constraint {
    std::optional<int64_t> min_value;
    std::optional<int64_t> max_value;
    bool min_inclusive = true;
    bool max_inclusive = true;
};

struct size_constraint {
    std::optional<size_t> min_size;
    std::optional<size_t> max_size;
};

struct permitted_alphabet_constraint {
    std::string alphabet;
};

struct extension_constraint {};

struct constraint {
    using variant_type = std::variant<
        value_range_constraint,
        size_constraint,
        permitted_alphabet_constraint,
        extension_constraint
    >;
    variant_type content;
};

struct constrained_type {
    std::unique_ptr<type_ref> underlying_type;
    std::vector<constraint> constraints;
};

struct null_value {};

struct value_ref {
    using variant_type = std::variant<
        int64_t,
        bool,
        std::string,
        null_value,
        std::vector<uint8_t>
    >;
    variant_type content;
};

struct type_assignment {
    std::string name;
    std::unique_ptr<type_ref> type;
};

struct value_assignment {
    std::string name;
    std::unique_ptr<type_ref> type;
    value_ref value;
};

struct type_from_object_assignment {
    std::string name;
    std::string object_name;
    std::string field_name;
};

struct assignment {
    using variant_type = std::variant<
        type_assignment,
        value_assignment,
        type_from_object_assignment
    >;
    variant_type content;

    assignment() = default;
    ~assignment() = default;
    assignment(assignment&&) noexcept = default;
    assignment& operator=(assignment&&) noexcept = default;
    assignment(const assignment&) = delete;
    assignment& operator=(const assignment&) = delete;
};

struct module_definition {
    std::string name;
    tag_default default_tagging = tag_default::automatic_tag;
    bool extensibility_implied = false;
    std::vector<assignment> assignments;
    std::string module_oid;
};

}  // namespace asn1pp::gen

template<>
struct asn1pp::tag_for_type<asn1pp::gen::integer_type> {
    static constexpr tag value = make_universal(universal_tag::integer);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::boolean_type> {
    static constexpr tag value = make_universal(universal_tag::boolean);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::null_type> {
    static constexpr tag value = make_universal(universal_tag::null);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::real_type> {
    static constexpr tag value = make_universal(universal_tag::real);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::octet_string_type> {
    static constexpr tag value = make_universal(universal_tag::octet_string);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::object_identifier_type> {
    static constexpr tag value = make_universal(universal_tag::object_identifier);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::relative_oid_type> {
    static constexpr tag value = make_universal(universal_tag::relative_oid);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::bit_string_type> {
    static constexpr tag value = make_universal(universal_tag::bit_string);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::enumerated_type> {
    static constexpr tag value = make_universal(universal_tag::enumerated);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::sequence_type> {
    static constexpr tag value = make_universal(universal_tag::sequence, true);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::set_type> {
    static constexpr tag value = make_universal(universal_tag::set, true);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::choice_type> {
    static constexpr tag value = make_universal(universal_tag::sequence, true);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::sequence_of_type> {
    static constexpr tag value = make_universal(universal_tag::sequence, true);
};

template<>
struct asn1pp::tag_for_type<asn1pp::gen::set_of_type> {
    static constexpr tag value = make_universal(universal_tag::set, true);
};

template<>
struct asn1pp::is_type_constructed<asn1pp::gen::sequence_type> : std::true_type {};

template<>
struct asn1pp::is_type_constructed<asn1pp::gen::set_type> : std::true_type {};

template<>
struct asn1pp::is_type_constructed<asn1pp::gen::choice_type> : std::true_type {};

template<>
struct asn1pp::is_type_constructed<asn1pp::gen::sequence_of_type> : std::true_type {};

template<>
struct asn1pp::is_type_constructed<asn1pp::gen::set_of_type> : std::true_type {};

template<>
struct asn1pp::is_type_primitive<asn1pp::gen::integer_type> : std::true_type {};

template<>
struct asn1pp::is_type_primitive<asn1pp::gen::boolean_type> : std::true_type {};

template<>
struct asn1pp::is_type_primitive<asn1pp::gen::null_type> : std::true_type {};

template<>
struct asn1pp::is_type_primitive<asn1pp::gen::real_type> : std::true_type {};

template<>
struct asn1pp::is_type_primitive<asn1pp::gen::octet_string_type> : std::true_type {};

template<>
struct asn1pp::is_type_primitive<asn1pp::gen::object_identifier_type> : std::true_type {};

template<>
struct asn1pp::is_type_primitive<asn1pp::gen::relative_oid_type> : std::true_type {};

template<>
struct asn1pp::is_type_primitive<asn1pp::gen::bit_string_type> : std::true_type {};

template<>
struct asn1pp::is_type_primitive<asn1pp::gen::enumerated_type> : std::true_type {};
