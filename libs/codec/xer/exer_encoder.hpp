#pragma once

#include "codec/result.hpp"
#include "codec/xer/encoder.hpp"

namespace asn1pp::xer {

// E-XER (Extended XER) encoder
// Adds XML attributes for ASN.1 metadata:
// 1. xmlns:asn1 namespace attribute
// 2. Type identification attributes
// 3. Support for XML namespaces

class exer_encoder {
public:
    static constexpr const char* ASN1_NS = "xmlns:asn1";
    static constexpr const char* ASN1_URI = "urn:asn1pp:asn1";

    static result<void> encode_sequence_start(const char* type_name, std::string& out) {
        out += "<Sequence";
        add_asn1_namespace(out);
        add_type_attribute(type_name, out);
        out += ">";
        return result<void>::ok();
    }

    // Add xmlns:asn1 namespace attribute
    static void add_asn1_namespace(std::string& out) {
        out += " ";
        out += ASN1_NS;
        out += "=\"";
        out += ASN1_URI;
        out += "\"";
    }

    // Add type identification attribute
    static void add_type_attribute(const char* type_name, std::string& out) {
        out += " asn1:type=\"";
        out += type_name;
        out += "\"";
    }

    // Encode element with type attribute
    static result<void> encode_element_with_type(const char* tag_name, const char* type_name,
                                                   const char* value, std::string& out) {
        out += "<";
        out += tag_name;
        out += " asn1:type=\"";
        out += type_name;
        out += "\">";
        out += value;
        out += "</";
        out += tag_name;
        out += ">";
        return result<void>::ok();
    }
};

} // namespace asn1pp::xer