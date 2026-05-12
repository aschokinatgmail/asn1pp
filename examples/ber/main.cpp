#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <iostream>
#include <iomanip>
#include <array>

#include "codec/ber/encoder.hpp"
#include "codec/ber/decoder.hpp"
#include "codec/traits.hpp"

using namespace asn1pp;
using namespace asn1pp::ber;

static void hex_dump(const char* label, std::span<const uint8_t> data) {
    std::cout << label << " (" << data.size() << " bytes): ";
    for (uint8_t b : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << "\n";
}

template<typename T>
static bool round_trip_ok(const char* name, T encoded, T decoded) {
    if (encoded == decoded) {
        std::cout << "[PASS] " << name << ": round-trip OK\n";
        return true;
    } else {
        std::cout << "[FAIL] " << name << ": mismatch (encoded=" << encoded
                  << ", decoded=" << decoded << ")\n";
        return false;
    }
}

int main() {
    std::cout << "=== BER Encode/Decode Tutorial ===\n\n";

    std::vector<uint8_t> enc(256);
    int failed = 0;

    {
        ber_encoder encoder;
        ber_decoder decoder_;

        int64_t value = 42;
        buffer_view view(enc);
        auto r = encoder.encode_integer(value, view);
        if (r.is_err()) {
            std::cout << "[FAIL] encode_integer(42): error_code=" << static_cast<int>(r.error()) << "\n";
            ++failed;
        } else {
            auto flush_result = encoder.flush_encode();
            if (flush_result != error_code::ok) {
                std::cout << "[FAIL] flush_encode: error_code=" << static_cast<int>(flush_result) << "\n";
                ++failed;
            } else {
                size_t encoded_size = 3;
                std::span<const uint8_t> encoded(enc.data(), encoded_size);
                hex_dump("INTEGER 42 encoded", encoded);

                buffer_view dec_view(encoded);
                auto dr = decoder_.decode_integer(dec_view);
                if (dr.is_err()) {
                    std::cout << "[FAIL] decode_integer: error_code=" << static_cast<int>(dr.error()) << "\n";
                    ++failed;
                } else if (!round_trip_ok("INTEGER 42", value, dr.value())) {
                    ++failed;
                }
            }
        }
    }

    {
        ber_encoder encoder;
        ber_decoder decoder_;

        std::array<uint8_t, 3> value = {0x01, 0x02, 0x03};
        buffer_view view(enc);
        auto r = encoder.encode_octet_string(value, view);
        if (r.is_err()) {
            std::cout << "[FAIL] encode_octet_string: error_code=" << static_cast<int>(r.error()) << "\n";
            ++failed;
        } else {
            size_t encoded_size = 5;
            std::span<const uint8_t> encoded(enc.data(), encoded_size);
            hex_dump("OCTET STRING {01 02 03} encoded", encoded);

            buffer_view dec_view(encoded);
            auto dr = decoder_.decode_octet_string(dec_view);
            if (dr.is_err()) {
                std::cout << "[FAIL] decode_octet_string: error_code=" << static_cast<int>(dr.error()) << "\n";
                ++failed;
            } else {
                auto dv = dr.value();
                if (dv.size() != value.size() ||
                    !std::equal(value.begin(), value.end(), dv.begin())) {
                    std::cout << "[FAIL] OCTET STRING: content mismatch\n";
                    ++failed;
                } else {
                    std::cout << "[PASS] OCTET STRING {01 02 03}: round-trip OK\n";
                }
            }
        }
    }

    {
        ber_encoder encoder;
        ber_decoder decoder_;

        std::array<int64_t, 3> values = {10, 20, 30};

        size_t header_size = 0;
        {
            buffer_view view(enc);
            auto hr = encoder.encode_sequence_header(
                make_universal(universal_tag::sequence),
                9,
                view
            );
            if (hr.is_err()) {
                std::cout << "[FAIL] encode_sequence_header: error_code="
                          << static_cast<int>(hr.error()) << "\n";
                ++failed;
            } else {
                header_size = hr.value();
            }
        }

        size_t pos = header_size;
        for (int64_t v : values) {
            buffer_view int_view(enc.data() + pos, enc.size() - pos);
            auto r = encoder.encode_integer(v, int_view);
            if (r.is_err()) {
                std::cout << "[FAIL] encode_integer in sequence: error_code="
                          << static_cast<int>(r.error()) << "\n";
                ++failed;
            }
            auto flush_result = encoder.flush_encode();
            if (flush_result != error_code::ok) {
                std::cout << "[FAIL] flush_encode: error_code=" << static_cast<int>(flush_result) << "\n";
                ++failed;
            }
            pos += 3;
        }

        size_t encoded_size = header_size + 9;
        std::span<const uint8_t> encoded(enc.data(), encoded_size);
        hex_dump("SEQUENCE {10, 20, 30} encoded", encoded);

        buffer_view dec_view(encoded);
        auto dr = decoder_.decode_tlv(dec_view);
        if (dr.is_err()) {
            std::cout << "[FAIL] decode_tlv: error_code=" << static_cast<int>(dr.error()) << "\n";
            ++failed;
        } else {
            auto& tlv = dr.value();
            std::cout << "[INFO] SEQUENCE: tag=" << static_cast<int>(tlv.t.number)
                      << " length=" << tlv.length << "\n";

            buffer_view content_view(tlv.value);
            bool seq_ok = true;
            for (size_t i = 0; i < values.size(); ++i) {
                auto ir = decoder_.decode_integer(content_view);
                if (ir.is_err()) {
                    std::cout << "[FAIL] decode_integer[" << i << "]: error_code="
                              << static_cast<int>(ir.error()) << "\n";
                    seq_ok = false;
                    ++failed;
                } else if (ir.value() != values[i]) {
                    std::cout << "[FAIL] decode_integer[" << i << "]: expected "
                              << values[i] << ", got " << ir.value() << "\n";
                    seq_ok = false;
                    ++failed;
                }
            }
            if (seq_ok) {
                std::cout << "[PASS] SEQUENCE {10, 20, 30}: round-trip OK\n";
            }
        }
    }

    std::cout << "\n=== Results ===\n";
    if (failed == 0) {
        std::cout << "All 3 round-trips passed.\n";
        return 0;
    } else {
        std::cout << failed << " test(s) failed.\n";
        return 1;
    }
}
