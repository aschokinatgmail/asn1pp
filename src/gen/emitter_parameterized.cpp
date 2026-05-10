#include "emitter_parameterized.hpp"
#include "emitter_sequence.hpp"

#include <sstream>
#include <variant>

#include "../../libs/codec/traits.hpp"

namespace asn1pp::gen {

namespace {

std::string sanitize_name(const std::string& name) {
    std::string result;
    for (char ch : name) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') || ch == '_') {
            result += ch;
        } else {
            result += '_';
        }
    }
    return result;
}

std::string cpp_type_for(const type_ref& type) {
    return std::visit(
        [](const auto& alt) -> std::string {
            using T = std::decay_t<decltype(alt)>;
            if constexpr (std::is_same_v<T, integer_type>) return "int64_t";
            else if constexpr (std::is_same_v<T, boolean_type>) return "bool";
            else if constexpr (std::is_same_v<T, null_type>) return "std::monostate";
            else if constexpr (std::is_same_v<T, real_type>) return "double";
            else if constexpr (std::is_same_v<T, octet_string_type>) return "std::vector<uint8_t>";
            else if constexpr (std::is_same_v<T, object_identifier_type>) return "std::vector<uint32_t>";
            else if constexpr (std::is_same_v<T, relative_oid_type>) return "std::vector<uint32_t>";
            else if constexpr (std::is_same_v<T, any_type>) return "std::vector<uint8_t>";
            else if constexpr (std::is_same_v<T, std::string>) return alt;
            else if constexpr (std::is_same_v<T, bit_string_type>) return "std::pair<std::vector<uint8_t>, size_t>";
            else if constexpr (std::is_same_v<T, enumerated_type>) return "int32_t";
            else if constexpr (std::is_same_v<T, std::unique_ptr<sequence_of_type>>) {
                return "std::vector<" + cpp_type_for(*alt->element_type) + ">";
            }
            else if constexpr (std::is_same_v<T, std::unique_ptr<set_of_type>>) {
                return "std::vector<" + cpp_type_for(*alt->element_type) + ">";
            }
            else if constexpr (std::is_same_v<T, std::unique_ptr<tagged_type>>) {
                return cpp_type_for(*alt->underlying_type);
            }
            else if constexpr (std::is_same_v<T, std::unique_ptr<constrained_type>>) {
                return cpp_type_for(*alt->underlying_type);
            }
            else if constexpr (std::is_same_v<T, std::unique_ptr<type_instantiation>>) {
                return alt->type_name;
            }
            return "void";
        },
        type.content);
}

bool is_type_parameter(const formal_parameter& fp) {
    return !fp.param_type.has_value();
}

std::string resolve_field_type_name(const type_ref& type,
                                     const std::vector<formal_parameter>& params,
                                     const std::vector<actual_parameter>& args) {
    if (auto* name = std::get_if<std::string>(&type.content)) {
        for (size_t i = 0; i < params.size() && i < args.size(); ++i) {
            if (params[i].name == *name && is_type_parameter(params[i])) {
                if (auto* tr = std::get_if<std::unique_ptr<type_ref>>(&args[i].value)) {
                    return cpp_type_for(**tr);
                }
                if (auto* val = std::get_if<std::string>(&args[i].value)) {
                    return *val;
                }
            }
        }
        return *name;
    }

    if (auto* inst = std::get_if<std::unique_ptr<type_instantiation>>(&type.content)) {
        return (*inst)->type_name;
    }

    return cpp_type_for(type);
}

std::string emit_sequence_with_substitution(
    const sequence_type& seq,
    const std::vector<formal_parameter>& params,
    const std::vector<actual_parameter>& args,
    std::string_view type_name)
{
    std::ostringstream result;
    result << "struct " << type_name << " {\n";

    for (const auto& comp : seq.components) {
        std::string cpp_type = resolve_field_type_name(*comp.type, params, args);
        std::string fname = sanitize_name(comp.name);

        if (comp.optional) {
            result << "    std::optional<" << cpp_type << "> " << fname << ";\n";
        } else if (comp.default_value.has_value()) {
            result << "    " << cpp_type << " " << fname << "{" << *comp.default_value << "};\n";
        } else {
            bool needs_brace = (cpp_type == "int64_t" || cpp_type == "bool" ||
                                cpp_type == "double" || cpp_type.find("std::vector") == 0 ||
                                cpp_type.find("std::variant") == 0 || cpp_type.find("std::pair") == 0);
            if (needs_brace) {
                result << "    " << cpp_type << " " << fname << "{};\n";
            } else {
                result << "    " << cpp_type << " " << fname << ";\n";
            }
        }
    }

    result << "\n    bool operator==(const " << type_name << "&) const = default;\n";
    result << "};\n\n";
    return result.str();
}

std::unique_ptr<type_ref> clone_type(const type_ref& src);

component_type clone_component(const component_type& src) {
    component_type c;
    c.name = src.name;
    c.type = clone_type(*src.type);
    c.optional = src.optional;
    c.default_value = src.default_value;
    return c;
}

choice_alternative clone_alternative(const choice_alternative& src) {
    choice_alternative a;
    a.name = src.name;
    a.type = clone_type(*src.type);
    return a;
}

std::unique_ptr<type_ref> clone_type(const type_ref& src) {
    auto dst = std::make_unique<type_ref>();
    std::visit([&](const auto& alt) {
        using T = std::decay_t<decltype(alt)>;
        if constexpr (std::is_same_v<T, std::unique_ptr<sequence_type>>) {
            auto seq = std::make_unique<sequence_type>();
            seq->has_extension = alt->has_extension;
            for (const auto& comp : alt->components) {
                seq->components.push_back(clone_component(comp));
            }
            dst->content = std::move(seq);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<set_type>>) {
            auto s = std::make_unique<set_type>();
            s->has_extension = alt->has_extension;
            for (const auto& comp : alt->components) {
                s->components.push_back(clone_component(comp));
            }
            dst->content = std::move(s);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<choice_type>>) {
            auto ch = std::make_unique<choice_type>();
            ch->has_extension = alt->has_extension;
            for (const auto& a : alt->alternatives) {
                ch->alternatives.push_back(clone_alternative(a));
            }
            dst->content = std::move(ch);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<sequence_of_type>>) {
            auto so = std::make_unique<sequence_of_type>();
            so->element_type = clone_type(*alt->element_type);
            dst->content = std::move(so);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<set_of_type>>) {
            auto so = std::make_unique<set_of_type>();
            so->element_type = clone_type(*alt->element_type);
            dst->content = std::move(so);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<tagged_type>>) {
            auto tt = std::make_unique<tagged_type>();
            tt->tag_value = alt->tag_value;
            tt->implicit = alt->implicit;
            tt->underlying_type = clone_type(*alt->underlying_type);
            dst->content = std::move(tt);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<constrained_type>>) {
            auto ct = std::make_unique<constrained_type>();
            ct->underlying_type = clone_type(*alt->underlying_type);
            ct->constraints = alt->constraints;
            dst->content = std::move(ct);
        } else if constexpr (std::is_same_v<T, std::unique_ptr<type_instantiation>>) {
            auto ti = std::make_unique<type_instantiation>();
            ti->type_name = alt->type_name;
            for (const auto& arg : alt->arguments) {
                actual_parameter ap;
                if (auto* tr = std::get_if<std::unique_ptr<type_ref>>(&arg.value)) {
                    ap.value = clone_type(**tr);
                } else if (auto* val = std::get_if<std::string>(&arg.value)) {
                    ap.value = *val;
                }
                ti->arguments.push_back(std::move(ap));
            }
            dst->content = std::move(ti);
        } else {
            dst->content = alt;
        }
    }, src.content);
    return dst;
}

void substitute_in_place(type_ref& type,
                          const std::vector<formal_parameter>& params,
                          const std::vector<actual_parameter>& args) {
    if (auto* name = std::get_if<std::string>(&type.content)) {
        for (size_t i = 0; i < params.size() && i < args.size(); ++i) {
            if (params[i].name == *name && is_type_parameter(params[i])) {
                if (auto* tr = std::get_if<std::unique_ptr<type_ref>>(&args[i].value)) {
                    auto replacement = clone_type(**tr);
                    type = std::move(*replacement);
                    return;
                }
            }
        }
        return;
    }

    if (auto* seq = std::get_if<std::unique_ptr<sequence_type>>(&type.content)) {
        for (auto& comp : (*seq)->components) {
            substitute_in_place(*comp.type, params, args);
        }
        return;
    }

    if (auto* ch = std::get_if<std::unique_ptr<choice_type>>(&type.content)) {
        for (auto& alt : (*ch)->alternatives) {
            substitute_in_place(*alt.type, params, args);
        }
        return;
    }

    if (auto* so = std::get_if<std::unique_ptr<sequence_of_type>>(&type.content)) {
        substitute_in_place(*(*so)->element_type, params, args);
        return;
    }

    if (auto* so = std::get_if<std::unique_ptr<set_of_type>>(&type.content)) {
        substitute_in_place(*(*so)->element_type, params, args);
        return;
    }

    if (auto* tt = std::get_if<std::unique_ptr<tagged_type>>(&type.content)) {
        substitute_in_place(*(*tt)->underlying_type, params, args);
        return;
    }

    if (auto* ct = std::get_if<std::unique_ptr<constrained_type>>(&type.content)) {
        substitute_in_place(*(*ct)->underlying_type, params, args);
        return;
    }
}

}  // namespace

std::string template_param_decl(const formal_parameter& fp) {
    if (is_type_parameter(fp)) {
        return "typename " + sanitize_name(fp.name);
    }
    return "int64_t " + sanitize_name(fp.name);
}

std::string template_arg_str(const actual_parameter& ap) {
    if (auto* tr = std::get_if<std::unique_ptr<type_ref>>(&ap.value)) {
        return cpp_type_for(**tr);
    }
    if (auto* val = std::get_if<std::string>(&ap.value)) {
        return *val;
    }
    return "void";
}

std::string emit_parameterized(const parameterized_type_assignment& pta, tag_default tagging) {
    std::ostringstream result;

    result << "template<";
    for (size_t i = 0; i < pta.parameters.size(); ++i) {
        if (i > 0) result << ", ";
        result << template_param_decl(pta.parameters[i]);
    }
    result << ">\n";

    if (pta.type->holds_ptr<sequence_type>()) {
        const auto& seq = pta.type->get_ptr<sequence_type>();
        result << "struct " << pta.name << " {\n";
        for (const auto& comp : seq.components) {
            std::string cpp_type;
            if (auto* name = std::get_if<std::string>(&comp.type->content)) {
                bool is_param = false;
                for (const auto& fp : pta.parameters) {
                    if (fp.name == *name) {
                        if (is_type_parameter(fp)) {
                            cpp_type = sanitize_name(fp.name);
                        } else {
                            cpp_type = cpp_type_for(*comp.type);
                        }
                        is_param = true;
                        break;
                    }
                }
                if (!is_param) {
                    cpp_type = *name;
                }
            } else {
                cpp_type = cpp_type_for(*comp.type);
            }

            std::string fname = sanitize_name(comp.name);
            if (comp.optional) {
                result << "    std::optional<" << cpp_type << "> " << fname << ";\n";
            } else if (comp.default_value.has_value()) {
                result << "    " << cpp_type << " " << fname << "{" << *comp.default_value << "};\n";
            } else {
                bool needs_brace = (cpp_type == "int64_t" || cpp_type == "bool" ||
                                    cpp_type == "double" || cpp_type.find("std::vector") == 0 ||
                                    cpp_type.find("std::pair") == 0);
                if (needs_brace) {
                    result << "    " << cpp_type << " " << fname << "{};\n";
                } else {
                    result << "    " << cpp_type << " " << fname << ";\n";
                }
            }
        }
        result << "\n    bool operator==(const " << pta.name << "&) const = default;\n";
        result << "};\n\n";
    } else if (pta.type->holds_ptr<choice_type>()) {
        const auto& choice = pta.type->get_ptr<choice_type>();
        result << "struct " << pta.name << " {\n";
        result << "    std::variant<";
        for (size_t i = 0; i < choice.alternatives.size(); ++i) {
            if (i > 0) result << ", ";
            std::string cpp_type;
            if (auto* name = std::get_if<std::string>(&choice.alternatives[i].type->content)) {
                bool is_param = false;
                for (const auto& fp : pta.parameters) {
                    if (fp.name == *name && is_type_parameter(fp)) {
                        cpp_type = sanitize_name(fp.name);
                        is_param = true;
                        break;
                    }
                }
                if (!is_param) cpp_type = *name;
            } else {
                cpp_type = cpp_type_for(*choice.alternatives[i].type);
            }
            result << cpp_type;
        }
        result << "> value;\n";
        result << "};\n\n";
    } else {
        result << "struct " << pta.name << " {\n";
        std::string cpp_type = cpp_type_for(*pta.type);
        result << "    " << cpp_type << " value;\n";
        result << "};\n\n";
    }

    (void)tagging;
    return result.str();
}

bool resolve_parameters(
    const type_ref& type,
    const std::vector<formal_parameter>& params,
    const std::vector<actual_parameter>& args,
    type_ref& resolved)
{
    auto cloned = clone_type(type);
    substitute_in_place(*cloned, params, args);
    resolved = std::move(*cloned);
    return true;
}

std::string emit_specialization(
    std::string_view base_name,
    const std::vector<formal_parameter>& params,
    const type_instantiation& inst,
    const type_ref& pta_type,
    tag_default tagging)
{
    std::ostringstream result;

    result << "template<>\n";
    result << "struct " << base_name << "<";
    for (size_t i = 0; i < inst.arguments.size(); ++i) {
        if (i > 0) result << ", ";
        result << template_arg_str(inst.arguments[i]);
    }
    result << ">";

    auto cloned = clone_type(pta_type);
    substitute_in_place(*cloned, params, inst.arguments);

    if (auto* seq = std::get_if<std::unique_ptr<sequence_type>>(&cloned->content)) {
        std::string spec_name = std::string(base_name) + "<";
        for (size_t i = 0; i < inst.arguments.size(); ++i) {
            if (i > 0) spec_name += ", ";
            spec_name += template_arg_str(inst.arguments[i]);
        }
        spec_name += ">";
        result << " " << emit_sequence_with_substitution(**seq, params, inst.arguments, spec_name);
    } else {
        result << " {\n";
        result << "    " << cpp_type_for(*cloned) << " value;\n";
        result << "};\n\n";
    }

    (void)tagging;
    return result.str();
}

}  // namespace asn1pp::gen
