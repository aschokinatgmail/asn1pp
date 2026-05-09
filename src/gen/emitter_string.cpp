#include "emitter_string.hpp"
#include "../../libs/codec/result.hpp"
#include "ast.hpp"

namespace asn1pp::gen {
namespace emitter {

namespace {

bool has_size_constraint(const constrained_type& ct) {
    for (const auto& c : ct.constraints) {
        if (std::holds_alternative<size_constraint>(c.content)) {
            return true;
        }
    }
    return false;
}

std::optional<size_constraint> get_size_constraint(const constrained_type& ct) {
    for (const auto& c : ct.constraints) {
        if (auto* sc = std::get_if<size_constraint>(&c.content)) {
            return *sc;
        }
    }
    return std::nullopt;
}

std::string sanitize_name(const std::string& name) {
    std::string result;
    for (char ch : name) {
        if (ch >= 'a' && ch <= 'z') result += ch;
        else if (ch >= 'A' && ch <= 'Z') result += ch;
        else if (ch >= '0' && ch <= '9') result += ch;
        else if (ch == '-' || ch == '_') result += ch;
        else result += '_';
    }
    return result;
}

} // namespace

emit_result emit_octet_string(const type_ref& type, const std::string& name) {
    std::string code;
    code += "struct " + name + " {\n";
    code += "    std::vector<uint8_t> value;\n";
    code += "    bool operator==(const " + name + "&) const = default;\n";

    if (type.holds_ptr<constrained_type>()) {
        const auto& ct = type.get_ptr<constrained_type>();
        if (has_size_constraint(ct)) {
            auto sc = get_size_constraint(ct);
            code += "    result<void> validate() const {\n";
            if (sc->min_size.has_value() && sc->max_size.has_value()) {
                code += "        if (value.size() < " + std::to_string(*sc->min_size) +
                        " || value.size() > " + std::to_string(*sc->max_size) +
                        ") return result<void>::err(error_code::constraint_violation);\n";
            } else if (sc->min_size.has_value()) {
                code += "        if (value.size() < " + std::to_string(*sc->min_size) +
                        ") return result<void>::err(error_code::constraint_violation);\n";
            } else if (sc->max_size.has_value()) {
                code += "        if (value.size() > " + std::to_string(*sc->max_size) +
                        ") return result<void>::err(error_code::constraint_violation);\n";
            }
            code += "        return result<void>::ok();\n";
            code += "    }\n";
        }
    }

    code += "};\n";
    code += "template<> struct asn1pp::asn1_tag<" + name + "> { static constexpr auto value = universal_tag::octet_string; };\n";

    return {code, true, {}};
}

emit_result emit_bit_string(const type_ref& type, const std::string& name) {
    std::string code;
    code += "struct " + name + " {\n";
    code += "    std::vector<uint8_t> data;\n";
    code += "    size_t bit_length{};\n";
    code += "    bool operator==(const " + name + "&) const = default;\n";

    if (type.holds_alternative<bit_string_type>()) {
        const auto& bt = type.get<bit_string_type>();
        if (!bt.named_bits.empty()) {
            for (const auto& nb : bt.named_bits) {
                std::string mname = sanitize_name(nb.name);
                if (nb.position.has_value()) {
                    size_t pos = static_cast<size_t>(*nb.position);
                    code += "    bool " + mname + "() const {\n";
                    code += "        if (" + std::to_string(pos) + " >= bit_length) return false;\n";
                    code += "        return (data[" + std::to_string(pos / 8) + "] & (0x80 >> (" + std::to_string(pos % 8) + "))) != 0;\n";
                    code += "    }\n";
                    code += "    void set_" + mname + "(bool v) {\n";
                    code += "        if (" + std::to_string(pos) + " >= bit_length) return;\n";
                    code += "        if (v) data[" + std::to_string(pos / 8) + "] |= (0x80 >> " + std::to_string(pos % 8) + ");\n";
                    code += "        else data[" + std::to_string(pos / 8) + "] &= ~(0x80 >> " + std::to_string(pos % 8) + ");\n";
                    code += "    }\n";
                }
            }
        }
    }

    code += "};\n";
    code += "template<> struct asn1pp::asn1_tag<" + name + "> { static constexpr auto value = universal_tag::bit_string; };\n";

    return {code, true, {}};
}

} // namespace emitter
} // namespace asn1pp::gen