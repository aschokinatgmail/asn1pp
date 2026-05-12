#include <cstdint>
#include <iostream>
#include <iomanip>
#include <vector>

#include "codec/ber/encoder.hpp"
#include "codec/ber/decoder.hpp"
#include "buffer/buffer_view.hpp"

using namespace asn1pp;

int main() {
    std::vector<uint8_t> encoded(16);
    buffer_view out(encoded);
    ber::ber_encoder enc;
    auto result = enc.encode_integer(42, out);

    if (result.is_err()) {
        std::cerr << "encode failed\n";
        return 1;
    }

    size_t original_size = 16;
    size_t remaining = out.size();
    size_t encoded_len = original_size - remaining;

    std::cout << "encoded: ";
    for (size_t i = 0; i < encoded_len; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(encoded[i]);
    }
    std::cout << "\n";

    buffer_view in(encoded.data(), encoded_len);
    ber::ber_decoder dec;

    auto decoded = dec.decode_integer(in);
    if (decoded.is_err()) {
        std::cerr << "decode failed\n";
        return 1;
    }

    std::cout << "decoded: " << std::dec << decoded.value() << "\n";

    if (decoded.value() == 42) {
        std::cout << "success: round-trip verified\n";
        return 0;
    } else {
        std::cerr << "failure: expected 42, got " << decoded.value() << "\n";
        return 1;
    }
}