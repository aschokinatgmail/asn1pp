#include "ast_print.hpp"
#include <sstream>
#include <iomanip>

namespace asn1pp::gen {

std::string to_string(const enumeration_item& item) {
    std::ostringstream oss;
    oss << item.name;
    if (item.value.has_value()) {
        oss << "(" << item.value.value() << ")";
    }
    return oss.str();
}

std::string to_string(const named_bit& nb) {
    std::ostringstream oss;
    oss << nb.name;
    if (nb.position.has_value()) {
        oss << "(" << nb.position.value() << ")";
    }
    return oss.str();
}

std::string tag_class_name(asn1pp::tag_class cls) {
    switch (cls) {
        case asn1pp::tag_class::universal:        return "universal";
        case asn1pp::tag_class::application:      return "application";
        case asn1pp::tag_class::context_specific: return "context-specific";
        case asn1pp::tag_class::private_class:    return "private";
    }
    return "unknown";
}

std::string to_string(const asn1pp::tag& t) {
    std::ostringstream oss;
    oss << "[" << tag_class_name(t.cls) << "] "
        << (t.constructed ? "CONSTRUCTED" : "PRIMITIVE")
        << " " << t.number;
    return oss.str();
}

std::string to_string(const value_range_constraint& cc) {
    std::ostringstream oss;
    oss << "VALUE_RANGE";
    if (cc.min_value.has_value()) {
        oss << " " << (cc.min_inclusive ? ">=" : ">") << " " << cc.min_value.value();
    }
    if (cc.max_value.has_value()) {
        oss << " " << (cc.max_inclusive ? "<=" : "<") << " " << cc.max_value.value();
    }
    return oss.str();
}

std::string to_string(const size_constraint& cc) {
    std::ostringstream oss;
    oss << "SIZE";
    if (cc.min_size.has_value()) {
        oss << " " << cc.min_size.value();
    }
    if (cc.max_size.has_value()) {
        oss << ".." << cc.max_size.value();
    }
    return oss.str();
}

std::string to_string(const constraint& c) {
    return std::visit([](const auto& cc) -> std::string {
        using T = std::decay_t<decltype(cc)>;
        if constexpr (std::is_same_v<T, value_range_constraint>) {
            return to_string(cc);
        } else if constexpr (std::is_same_v<T, size_constraint>) {
            return to_string(cc);
        } else if constexpr (std::is_same_v<T, permitted_alphabet_constraint>) {
            return "PERMITTED_ALPHABET(" + cc.alphabet + ")";
        } else if constexpr (std::is_same_v<T, extension_constraint>) {
            return "EXTENSION";
        }
        return "UNKNOWN_CONSTRAINT";
    }, c.content);
}

std::string to_string(const component_type& comp) {
    std::ostringstream oss;
    oss << comp.name << " " << (comp.type ? to_string(*comp.type) : "<null>");
    if (comp.optional) {
        oss << " OPTIONAL";
    }
    if (comp.default_value.has_value()) {
        oss << " DEFAULT " << comp.default_value.value();
    }
    return oss.str();
}

std::string to_string(const choice_alternative& alt) {
    std::ostringstream oss;
    oss << alt.name << " " << (alt.type ? to_string(*alt.type) : "<null>");
    return oss.str();
}

std::string to_string(const sequence_type& t) {
    std::ostringstream oss;
    oss << "SEQUENCE { ";
    for (size_t i = 0; i < t.components.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << to_string(t.components[i]);
    }
    oss << " }";
    if (t.has_extension) {
        oss << " ..." ;
    }
    return oss.str();
}

std::string to_string(const set_type& t) {
    std::ostringstream oss;
    oss << "SET { ";
    for (size_t i = 0; i < t.components.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << to_string(t.components[i]);
    }
    oss << " }";
    if (t.has_extension) {
        oss << " ..." ;
    }
    return oss.str();
}

std::string to_string(const choice_type& t) {
    std::ostringstream oss;
    oss << "CHOICE { ";
    for (size_t i = 0; i < t.alternatives.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << to_string(t.alternatives[i]);
    }
    oss << " }";
    if (t.has_extension) {
        oss << " ..." ;
    }
    return oss.str();
}

std::string to_string(const sequence_of_type& t) {
    return "SEQUENCE OF " + (t.element_type ? to_string(*t.element_type) : "<null>");
}

std::string to_string(const set_of_type& t) {
    return "SET OF " + (t.element_type ? to_string(*t.element_type) : "<null>");
}

std::string to_string(const tagged_type& t) {
    std::ostringstream oss;
    oss << "TAG "
        << to_string(t.tag_value)
        << " "
        << (t.implicit ? "IMPLICIT" : "EXPLICIT")
        << " " << (t.underlying_type ? to_string(*t.underlying_type) : "<null>");
    return oss.str();
}

std::string to_string(const constrained_type& t) {
    std::ostringstream oss;
    oss << (t.underlying_type ? to_string(*t.underlying_type) : "<null>");
    for (const auto& c : t.constraints) {
        oss << " " << to_string(c);
    }
    return oss.str();
}

std::string to_string(const selection_type& t) {
    return "SELECTION " + t.field_name + " " + (t.selected_type ? to_string(*t.selected_type) : "<null>");
}

std::string to_string(const enumerated_type& t) {
    std::ostringstream oss;
    oss << "ENUMERATED { ";
    for (size_t i = 0; i < t.values.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << to_string(t.values[i]);
    }
    oss << " }";
    if (t.has_extension) {
        oss << " ..." ;
    }
    return oss.str();
}

std::string to_string(const bit_string_type& t) {
    std::ostringstream oss;
    oss << "BIT STRING { ";
    for (size_t i = 0; i < t.named_bits.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << to_string(t.named_bits[i]);
    }
    oss << " }";
    if (t.has_extension) {
        oss << " ..." ;
    }
    return oss.str();
}

std::string to_string(const type_instantiation& ti) {
    std::ostringstream oss;
    oss << ti.type_name;
    if (!ti.arguments.empty()) {
        oss << "{";
        for (size_t i = 0; i < ti.arguments.size(); ++i) {
            if (i > 0) oss << ", ";
            std::visit([&oss](const auto& v) {
                using V = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<V, std::string>) {
                    oss << v;
                } else if constexpr (std::is_same_v<V, std::unique_ptr<type_ref>>) {
                    oss << (v ? to_string(*v) : "<null>");
                }
            }, ti.arguments[i].value);
        }
        oss << "}";
    }
    return oss.str();
}

std::string to_string(const formal_parameter& fp) {
    std::ostringstream oss;
    if (fp.param_type) {
        oss << *fp.param_type << " : " << fp.name;
    } else {
        oss << fp.name;
    }
    if (fp.default_value) {
        oss << " DEFAULT " << *fp.default_value;
    }
    return oss.str();
}

std::string to_string(const parameterized_type_assignment& pta) {
    std::ostringstream oss;
    oss << "PARAMETERIZED-TYPE " << pta.name << " { ";
    for (size_t i = 0; i < pta.parameters.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << to_string(pta.parameters[i]);
    }
    oss << " } = " << (pta.type ? to_string(*pta.type) : "<null>");
    return oss.str();
}

std::string to_string(const class_type& ct) {
    std::ostringstream oss;
    oss << "CLASS " << ct.name << " { ";
    for (size_t i = 0; i < ct.fields.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << ct.fields[i].name;
        if (ct.fields[i].type_name) {
            oss << " " << *ct.fields[i].type_name;
        }
        if (ct.fields[i].optional) oss << " OPTIONAL";
        if (ct.fields[i].unique) oss << " UNIQUE";
    }
    oss << " }";
    if (!ct.with_syntax.empty()) {
        oss << " WITH SYNTAX { ";
        for (size_t i = 0; i < ct.with_syntax.size(); ++i) {
            oss << ct.with_syntax[i].literal;
            if (ct.with_syntax[i].field_ref) {
                oss << " " << *ct.with_syntax[i].field_ref;
            }
            if (i + 1 < ct.with_syntax.size()) oss << " ";
        }
        oss << " }";
    }
    return oss.str();
}

std::string to_string(const information_object& obj) {
    std::ostringstream oss;
    oss << "OBJECT " << obj.name << " : " << obj.class_name;
    if (!obj.field_values.empty()) {
        oss << " { ";
        for (size_t i = 0; i < obj.field_values.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << obj.field_values[i].field_name;
        }
        oss << " }";
    }
    return oss.str();
}

std::string to_string(const object_set& os) {
    std::ostringstream oss;
    oss << "OBJECT-SET " << os.name << " : " << os.class_name << " { ";
    for (size_t i = 0; i < os.objects.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << os.objects[i];
    }
    if (os.has_extension) oss << ", ...";
    oss << " }";
    return oss.str();
}

std::string to_string(const type_ref& type) {
    return std::visit([](const auto& t) -> std::string {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, integer_type>) {
            return "INTEGER";
        } else if constexpr (std::is_same_v<T, boolean_type>) {
            return "BOOLEAN";
        } else if constexpr (std::is_same_v<T, null_type>) {
            return "NULL";
        } else if constexpr (std::is_same_v<T, real_type>) {
            return "REAL";
        } else if constexpr (std::is_same_v<T, octet_string_type>) {
            return "OCTET STRING";
        } else if constexpr (std::is_same_v<T, object_identifier_type>) {
            return "OBJECT IDENTIFIER";
        } else if constexpr (std::is_same_v<T, relative_oid_type>) {
            return "RELATIVE-OID";
        } else if constexpr (std::is_same_v<T, any_type>) {
            return "ANY";
        } else if constexpr (std::is_same_v<T, std::string>) {
            return t;
        } else if constexpr (std::is_same_v<T, sequence_type>) {
            return to_string(t);
        } else if constexpr (std::is_same_v<T, set_type>) {
            return to_string(t);
        } else if constexpr (std::is_same_v<T, choice_type>) {
            return to_string(t);
        } else if constexpr (std::is_same_v<T, enumerated_type>) {
            return to_string(t);
        } else if constexpr (std::is_same_v<T, bit_string_type>) {
            return to_string(t);
        } else if constexpr (std::is_same_v<T, sequence_of_type>) {
            return to_string(t);
        } else if constexpr (std::is_same_v<T, set_of_type>) {
            return to_string(t);
        } else if constexpr (std::is_same_v<T, tagged_type>) {
            return to_string(t);
        } else if constexpr (std::is_same_v<T, constrained_type>) {
            return to_string(t);
        } else if constexpr (std::is_same_v<T, type_instantiation>) {
            return to_string(t);
        }
        return "UNKNOWN_TYPE";
    }, type.content);
}

std::string to_string(const module_definition& module) {
    std::ostringstream oss;
    oss << "MODULE " << module.name << "\n";
    oss << "OID " << module.module_oid << "\n";
    switch (module.default_tagging) {
        case tag_default::explicit_tag:   oss << "EXPLICIT TAGS\n"; break;
        case tag_default::implicit_tag:   oss << "IMPLICIT TAGS\n"; break;
        case tag_default::automatic_tag:  oss << "AUTOMATIC TAGS\n"; break;
    }
    if (module.extensibility_implied) {
        oss << "EXTENSIBILITY IMPLIED\n";
    }
    for (const auto& assg : module.assignments) {
        std::visit([&oss](const auto& a) {
            using T = std::decay_t<decltype(a)>;
            if constexpr (std::is_same_v<T, type_assignment>) {
                oss << "TYPE-ASSIGNMENT " << a.name << " = " << (a.type ? to_string(*a.type) : "<null>") << "\n";
            } else if constexpr (std::is_same_v<T, value_assignment>) {
                oss << "VALUE-ASSIGNMENT " << a.name << " " << (a.type ? to_string(*a.type) : "<null>") << "\n";
            } else if constexpr (std::is_same_v<T, type_from_object_assignment>) {
                oss << "TYPE-FROM-OBJECT " << a.name << "\n";
            } else if constexpr (std::is_same_v<T, parameterized_type_assignment>) {
                oss << to_string(a) << "\n";
            } else if constexpr (std::is_same_v<T, class_type>) {
                oss << to_string(a) << "\n";
            } else if constexpr (std::is_same_v<T, information_object>) {
                oss << to_string(a) << "\n";
            } else if constexpr (std::is_same_v<T, object_set>) {
                oss << to_string(a) << "\n";
            }
        }, assg.content);
    }
    return oss.str();
}

}  // namespace asn1pp::gen
