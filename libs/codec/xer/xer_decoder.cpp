#include "codec/xer/decoder.hpp"

#ifndef ASN1PP_NO_TEXT_CODECS

#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <utility>

#include "codec/result.hpp"
#include "arch/simd.hpp"

namespace asn1pp::xer {

std::vector<uint8_t> base64_decode(const std::string& encoded) {
    constexpr const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<uint8_t> result;
    int val = 0;
    int bits = 0;
    for (char c : encoded) {
        if (c == '=') break;
        const char* p = std::strchr(alphabet, c);
        if (!p) continue;
        val = (val << 6) | (p - alphabet);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            result.push_back(static_cast<uint8_t>(val >> bits));
            val &= (1 << bits) - 1;
        }
    }
    return result;
}

}  // namespace asn1pp::xer

#endif  // ASN1PP_NO_TEXT_CODECS