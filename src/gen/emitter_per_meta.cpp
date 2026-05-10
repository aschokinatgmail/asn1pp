#include "emitter_per_meta.hpp"

#include <sstream>

namespace asn1pp::gen {

namespace {

void emit_bool_array(std::ostringstream& oss, const std::vector<bool>& values) {
    oss << "{";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << (values[i] ? "true" : "false");
    }
    oss << "}";
}

const value_range_constraint* find_range_constraint(const std::vector<constraint>& constraints) {
    for (const auto& c : constraints) {
        if (std::holds_alternative<value_range_constraint>(c.content)) {
            return &std::get<value_range_constraint>(c.content);
        }
    }
    return nullptr;
}

const size_constraint* find_size_constraint(const std::vector<constraint>& constraints) {
    for (const auto& c : constraints) {
        if (std::holds_alternative<size_constraint>(c.content)) {
            return &std::get<size_constraint>(c.content);
        }
    }
    return nullptr;
}

bool emit_integer_per_meta(std::ostringstream& oss,
                            std::string_view type_name,
                            const std::vector<constraint>& constraints) {
    const auto* vrc = find_range_constraint(constraints);

    oss << "struct " << type_name << "_per_meta {\n";

    if (vrc) {
        oss << "    static constexpr int64_t min_value = "
            << (vrc->min_value.has_value() ? std::to_string(*vrc->min_value) : "0") << ";\n";
        oss << "    static constexpr int64_t max_value = "
            << (vrc->max_value.has_value() ? std::to_string(*vrc->max_value) : std::to_string(INT64_MAX)) << ";\n";
        oss << "    static constexpr bool has_range_constraint = true;\n";
        oss << "    static constexpr bool is_extension_permitted = false;\n";
    } else {
        oss << "    static constexpr int64_t min_value = 0;\n";
        oss << "    static constexpr int64_t max_value = " << INT64_MAX << ";\n";
        oss << "    static constexpr bool has_range_constraint = false;\n";
        oss << "    static constexpr bool is_extension_permitted = false;\n";
    }

    oss << "};\n";
    return true;
}

bool emit_octet_string_per_meta(std::ostringstream& oss,
                                 std::string_view type_name,
                                 const std::vector<constraint>& constraints) {
    const auto* sc = find_size_constraint(constraints);

    oss << "struct " << type_name << "_per_meta {\n";

    if (sc) {
        oss << "    static constexpr size_t min_size = "
            << (sc->min_size.has_value() ? std::to_string(*sc->min_size) : "0") << ";\n";
        oss << "    static constexpr size_t max_size = "
            << (sc->max_size.has_value() ? std::to_string(*sc->max_size) : std::to_string(SIZE_MAX)) << ";\n";
        oss << "    static constexpr bool has_size_constraint = true;\n";
        oss << "    static constexpr bool is_extension_permitted = false;\n";
    } else {
        oss << "    static constexpr size_t min_size = 0;\n";
        oss << "    static constexpr size_t max_size = " << SIZE_MAX << ";\n";
        oss << "    static constexpr bool has_size_constraint = false;\n";
        oss << "    static constexpr bool is_extension_permitted = false;\n";
    }

    oss << "};\n";
    return true;
}

bool emit_enumerated_per_meta(std::ostringstream& oss,
                               std::string_view type_name,
                               const enumerated_type& et) {
    oss << "struct " << type_name << "_per_meta {\n";
    oss << "    static constexpr size_t normal_index_count = " << et.values.size() << ";\n";
    oss << "    static constexpr bool has_extension = " << (et.has_extension ? "true" : "false") << ";\n";
    oss << "};\n";
    return true;
}

bool emit_sequence_per_meta(std::ostringstream& oss,
                             std::string_view type_name,
                             const sequence_type& seq) {
    std::vector<bool> bitmap;
    bitmap.reserve(seq.components.size());
    for (const auto& comp : seq.components) {
        bitmap.push_back(comp.optional);
    }

    oss << "struct " << type_name << "_per_meta {\n";
    oss << "    static constexpr size_t field_count = " << seq.components.size() << ";\n";
    if (seq.components.empty()) {
        oss << "    static constexpr bool optional_bitmap[0]";
    } else {
        oss << "    static constexpr bool optional_bitmap[" << bitmap.size() << "]";
    }
    oss << " = ";
    emit_bool_array(oss, bitmap);
    oss << ";\n";
    oss << "    static constexpr bool has_extension = " << (seq.has_extension ? "true" : "false") << ";\n";
    oss << "};\n";
    return true;
}

bool emit_choice_per_meta(std::ostringstream& oss,
                           std::string_view type_name,
                           const choice_type& ch) {
    oss << "struct " << type_name << "_per_meta {\n";
    oss << "    static constexpr size_t alternative_count = " << ch.alternatives.size() << ";\n";
    oss << "    static constexpr bool has_extension = " << (ch.has_extension ? "true" : "false") << ";\n";
    oss << "};\n";
    return true;
}

bool emit_set_per_meta(std::ostringstream& oss,
                        std::string_view type_name,
                        const set_type& set) {
    std::vector<bool> bitmap;
    bitmap.reserve(set.components.size());
    for (const auto& comp : set.components) {
        bitmap.push_back(comp.optional);
    }

    oss << "struct " << type_name << "_per_meta {\n";
    oss << "    static constexpr size_t field_count = " << set.components.size() << ";\n";
    if (set.components.empty()) {
        oss << "    static constexpr bool optional_bitmap[0]";
    } else {
        oss << "    static constexpr bool optional_bitmap[" << bitmap.size() << "]";
    }
    oss << " = ";
    emit_bool_array(oss, bitmap);
    oss << ";\n";
    oss << "    static constexpr bool has_extension = " << (set.has_extension ? "true" : "false") << ";\n";
    oss << "};\n";
    return true;
}

}  // anonymous namespace

std::string emit_per_meta(const type_ref& type,
                           std::string_view type_name,
                           const emitter_options&) {
    std::ostringstream oss;

    if (type.holds_ptr<constrained_type>()) {
        const auto& ct = type.get_ptr<constrained_type>();
        const auto& underlying = *ct.underlying_type;

        if (underlying.holds_alternative<integer_type>()) {
            emit_integer_per_meta(oss, type_name, ct.constraints);
            return oss.str();
        }
        if (underlying.holds_alternative<octet_string_type>()) {
            emit_octet_string_per_meta(oss, type_name, ct.constraints);
            return oss.str();
        }
    }

    if (type.holds_alternative<enumerated_type>()) {
        emit_enumerated_per_meta(oss, type_name, type.get<enumerated_type>());
        return oss.str();
    }

    if (type.holds_ptr<sequence_type>()) {
        emit_sequence_per_meta(oss, type_name, type.get_ptr<sequence_type>());
        return oss.str();
    }

    if (type.holds_ptr<choice_type>()) {
        emit_choice_per_meta(oss, type_name, type.get_ptr<choice_type>());
        return oss.str();
    }

    if (type.holds_ptr<set_type>()) {
        emit_set_per_meta(oss, type_name, type.get_ptr<set_type>());
        return oss.str();
    }

    if (type.holds_alternative<integer_type>()) {
        emit_integer_per_meta(oss, type_name, {});
        return oss.str();
    }

    if (type.holds_alternative<octet_string_type>()) {
        emit_octet_string_per_meta(oss, type_name, {});
        return oss.str();
    }

    oss << "struct " << type_name << "_per_meta {};\n";
    return oss.str();
}

}  // namespace asn1pp::gen
