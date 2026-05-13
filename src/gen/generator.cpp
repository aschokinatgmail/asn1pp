#include "generator.hpp"

#include "emitter_choice.hpp"
#include "emitter_enumerated.hpp"
#include "emitter_integer.hpp"
#include "emitter_null.hpp"
#include "emitter_oid.hpp"
#include "emitter_parameterized.hpp"
#include "emitter_sequence.hpp"
#include "emitter_sequence_of.hpp"
#include "emitter_string.hpp"
#include "emitter_tagged.hpp"

#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include <cctype>

namespace asn1pp::gen {
namespace {

template<typename T>
size_t type_index() {
    type_ref ref;
    ref.content = T{};
    return ref.content.index();
}

template<typename T>
size_t ptr_type_index() {
    type_ref ref;
    ref.content = std::make_unique<T>();
    return ref.content.index();
}

asn1pp::result<std::string> ok(std::string code) {
    return asn1pp::result<std::string>::ok(std::move(code));
}

asn1pp::result<std::string> parse_error() {
    return asn1pp::result<std::string>::err(asn1pp::error_code::parse_error);
}

std::string emit_boolean_standalone(const std::string& name) {
    std::string result;
    result += "struct " + name + " {\n";
    result += "    bool value{};\n";
    result += "    operator bool() const { return value; }\n";
    result += "    bool operator==(const " + name + "&) const = default;\n";
    result += "};\n";
    result += "template<> struct asn1pp::asn1_tag<" + name;
    result += "> { static constexpr auto value = asn1pp::universal_tag::boolean; };\n";
    return result;
}

std::string emit_real_standalone(const std::string& name) {
    std::string result;
    result += "struct " + name + " {\n";
    result += "    double value{};\n";
    result += "    bool operator==(const " + name + "&) const = default;\n";
    result += "};\n";
    result += "template<> struct asn1pp::asn1_tag<" + name;
    result += "> { static constexpr auto value = asn1pp::universal_tag::real; };\n";
    return result;
}

std::string emit_any_standalone(const std::string& name) {
    std::string result;
    result += "struct " + name + " {\n";
    result += "    std::vector<uint8_t> value;\n";
    result += "    bool operator==(const " + name + "&) const = default;\n";
    result += "};\n";
    return result;
}

std::string qualify_tag_specializations(std::string code, const std::string& ns) {
    if (ns.empty()) {
        return code;
    }

    const std::string needle = "asn1pp::asn1_tag<";
    std::string::size_type pos = 0;
    while ((pos = code.find(needle, pos)) != std::string::npos) {
        pos += needle.size();
        if (code.compare(pos, ns.size() + 2, ns + "::") != 0) {
            code.insert(pos, ns + "::");
            pos += ns.size() + 2;
        }
    }
    return code;
}

std::string header_guard_for_module(const std::string& name) {
    std::string result = "ASN1PP_";
    for (char c : name) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        } else {
            result += '_';
        }
    }
    result += "_HPP";
    return result;
}

std::unique_ptr<type_ref> copy_supported_type_ref(const type_ref& type) {
    return std::visit(
        [](const auto& alt) -> std::unique_ptr<type_ref> {
            using T = std::decay_t<decltype(alt)>;
            if constexpr (std::is_same_v<T, integer_type> ||
                          std::is_same_v<T, boolean_type> ||
                          std::is_same_v<T, null_type> ||
                          std::is_same_v<T, real_type> ||
                          std::is_same_v<T, octet_string_type> ||
                          std::is_same_v<T, object_identifier_type> ||
                          std::is_same_v<T, relative_oid_type> ||
                          std::is_same_v<T, any_type> ||
                          std::is_same_v<T, enumerated_type> ||
                          std::is_same_v<T, bit_string_type>) {
                auto ref = std::make_unique<type_ref>();
                ref->content = alt;
                return ref;
            } else {
                return nullptr;
            }
        },
        type.content);
}

}  // namespace

generator::generator() {
    register_emitter(type_index<integer_type>(),
        [](const type_assignment& assignment, const emitter_options& opts, tag_default) {
            if (assignment.type == nullptr || !assignment.type->holds_alternative<integer_type>()) return parse_error();
            return ok(emit_integer_type(assignment.type->get<integer_type>(), assignment.name, {}, opts));
        });

    register_emitter(type_index<boolean_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default) {
            return ok(emit_boolean_standalone(assignment.name));
        });

    register_emitter(type_index<null_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default) {
            return ok(emit_null_standalone(assignment.name));
        });

    register_emitter(type_index<real_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default) {
            return ok(emit_real_standalone(assignment.name));
        });

    register_emitter(type_index<octet_string_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default) {
            if (assignment.type == nullptr) return parse_error();
            auto result = emitter::emit_octet_string(*assignment.type, assignment.name);
            if (!result.success) return parse_error();
            return ok(std::move(result.code));
        });

    register_emitter(type_index<object_identifier_type>(),
        [](const type_assignment& assignment, const emitter_options& opts, tag_default) {
            if (assignment.type == nullptr || !assignment.type->holds_alternative<object_identifier_type>()) return parse_error();
            return ok(emit_object_identifier_type(assignment.type->get<object_identifier_type>(), assignment.name, {}, opts));
        });

    register_emitter(type_index<relative_oid_type>(),
        [](const type_assignment& assignment, const emitter_options& opts, tag_default) {
            if (assignment.type == nullptr || !assignment.type->holds_alternative<relative_oid_type>()) return parse_error();
            return ok(emit_relative_oid_type(assignment.type->get<relative_oid_type>(), assignment.name, {}, opts));
        });

    register_emitter(type_index<any_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default) {
            return ok(emit_any_standalone(assignment.name));
        });

    register_emitter(ptr_type_index<sequence_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default tagging) {
            if (assignment.type == nullptr || !assignment.type->holds_ptr<sequence_type>()) return parse_error();
            return ok(emit_sequence(assignment.name, assignment.type->get_ptr<sequence_type>(), tagging));
        });

    register_emitter(ptr_type_index<set_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default tagging) {
            if (assignment.type == nullptr || !assignment.type->holds_ptr<set_type>()) return parse_error();
            return ok(emit_set(assignment.name, assignment.type->get_ptr<set_type>(), tagging));
        });

    register_emitter(ptr_type_index<choice_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default tagging) {
            if (assignment.type == nullptr || !assignment.type->holds_ptr<choice_type>()) return parse_error();
            return ok(emit_choice(assignment.name, assignment.type->get_ptr<choice_type>(), tagging));
        });

    register_emitter(type_index<enumerated_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default) {
            if (assignment.type == nullptr || !assignment.type->holds_alternative<enumerated_type>()) return parse_error();
            return ok(emit_enumerated(assignment.name, assignment.type->get<enumerated_type>()));
        });

    register_emitter(type_index<bit_string_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default) {
            if (assignment.type == nullptr) return parse_error();
            auto result = emitter::emit_bit_string(*assignment.type, assignment.name);
            if (!result.success) return parse_error();
            return ok(std::move(result.code));
        });

    register_emitter(ptr_type_index<sequence_of_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default) {
            if (assignment.type == nullptr || !assignment.type->holds_ptr<sequence_of_type>()) return parse_error();
            return ok(emit_sequence_of(assignment.name, assignment.type->get_ptr<sequence_of_type>(), {}));
        });

    register_emitter(ptr_type_index<set_of_type>(),
        [](const type_assignment& assignment, const emitter_options&, tag_default) {
            if (assignment.type == nullptr || !assignment.type->holds_ptr<set_of_type>()) return parse_error();
            return ok(emit_set_of(assignment.name, assignment.type->get_ptr<set_of_type>(), {}));
        });

    register_emitter(ptr_type_index<tagged_type>(),
        [](const type_assignment& assignment, const emitter_options& opts, tag_default) {
            if (assignment.type == nullptr || !assignment.type->holds_ptr<tagged_type>()) return parse_error();
            return ok(emit_tagged_type(assignment.name, assignment.type->get_ptr<tagged_type>(), opts));
        });
}

asn1pp::result<void> generator::generate(const module_definition& module,
                                         const emitter_options& opts,
                                         std::ostream& os) {
    if (module.assignments.empty()) {
        return asn1pp::result<void>::err(asn1pp::error_code::parse_error);
    }

    std::ostringstream body;
    bool emitted = false;

    for (const auto& assignment : module.assignments) {
        if (std::holds_alternative<type_assignment>(assignment.content)) {
            const auto& type = std::get<type_assignment>(assignment.content);
            auto result = dispatch(type, opts, module.default_tagging);
            if (result.is_err()) {
                return asn1pp::result<void>::err(result.error());
            }
            body << qualify_tag_specializations(result.value(), opts.namespace_name) << '\n';
            emitted = true;
        } else if (std::holds_alternative<parameterized_type_assignment>(assignment.content)) {
            body << emit_parameterized(std::get<parameterized_type_assignment>(assignment.content),
                                       module.default_tagging) << '\n';
            emitted = true;
        }
    }

    if (!emitted) {
        return asn1pp::result<void>::err(asn1pp::error_code::parse_error);
    }

    const std::string guard = header_guard_for_module(module.name);
    os << "#ifndef " << guard << "\n";
    os << "#define " << guard << "\n\n";
    os << code_emitter::emit_includes({"asn1pp/codec.hpp", "asn1pp/traits.hpp", "<cstdint>", "<string>", "<variant>", "<vector>"});
    os << "\n";
    os << code_emitter::emit_namespace_open(opts.namespace_name);
    os << body.str();
    os << code_emitter::emit_namespace_close(opts.namespace_name);
    os << "\n#endif // " << guard << "\n";

    return asn1pp::result<void>::ok();
}

void generator::register_emitter(size_t type_index, emitter_fn fn) {
    registry_[type_index] = std::move(fn);
}

asn1pp::result<std::string> generator::dispatch(const type_assignment& ta,
                                                const emitter_options& opts,
                                                tag_default tagging) const {
    if (ta.type == nullptr) {
        return asn1pp::result<std::string>::err(asn1pp::error_code::parse_error);
    }

    if (ta.type->holds_alternative<std::string>()) {
        return asn1pp::result<std::string>::err(asn1pp::error_code::invalid_tag);
    }

    if (ta.type->holds_ptr<constrained_type>()) {
        const auto& constrained = ta.type->get_ptr<constrained_type>();
        if (constrained.underlying_type == nullptr) {
            return asn1pp::result<std::string>::err(asn1pp::error_code::parse_error);
        }

        auto unwrapped_type = copy_supported_type_ref(*constrained.underlying_type);
        if (unwrapped_type == nullptr) {
            return asn1pp::result<std::string>::err(asn1pp::error_code::invalid_tag);
        }

        type_assignment unwrapped;
        unwrapped.name = ta.name;
        unwrapped.type = std::move(unwrapped_type);

        auto found = registry_.find(constrained.underlying_type->content.index());
        if (found == registry_.end()) {
            return asn1pp::result<std::string>::err(asn1pp::error_code::invalid_tag);
        }
        return found->second(unwrapped, opts, tagging);
    }

    auto found = registry_.find(ta.type->content.index());
    if (found == registry_.end()) {
        return asn1pp::result<std::string>::err(asn1pp::error_code::invalid_tag);
    }
    return found->second(ta, opts, tagging);
}

}  // namespace asn1pp::gen
