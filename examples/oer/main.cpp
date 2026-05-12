// examples/oer/main.cpp — OER encode/decode tutorial example
//
// This example demonstrates Octet Encoding Rules (OER) encoding/decoding
// of a constrained integer (0–255). OER produces compact, fixed-width
// output when range constraints are known at compile time.
//
// Contrast with BER: BER encodes 42 as { 0x02, 0x01, 0x2A } (3 bytes, with tag+length).
//              OER encodes 42 as       { 0x01, 0x2A } (2 bytes, no tag, no length prefix).
//
// OER is defined in ITU-T X.696 (also known as "OSER").

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "codec/oer/encoder.hpp"
#include "codec/oer/decoder.hpp"
#include "codec/batch_buffer.hpp"
#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"

using namespace asn1pp;

// Int0To255 — OER metadata for a constrained integer [0..255]
// The has_range_constraint=true flag tells OER to use fixed-width encoding.
// Width is determined by the range size: ≤256 → 1 octet.
struct Int0To255 {
    static constexpr bool has_range_constraint = true;
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr bool extensible = false;
    // Size constraints are for SEQUENCE/OCTET STRING, not INTEGER range
    static constexpr size_t min_size = 0;
    static constexpr size_t max_size = 255;
    static constexpr bool has_size_constraint = true;
};

int main() {
    // ── Encode 42 → bytes [0x2A] ────────────────────────────────────────
    uint8_t oer_out[16] = {};
    buffer_view obv(oer_out, sizeof(oer_out));
    oer::oer_encoder enc;
    auto enc_result = enc.encode_integer<Int0To255>(42, obv);
    if (enc_result.is_err()) {
        printf("ERROR: encode failed with code %d\n", enc_result.error());
        return 1;
    }
    enc.flush_encode();
    const size_t encoded_len = 16 - obv.size();
    printf("Encoded %d bytes: ", (int)encoded_len);
    for (size_t i = 0; i < encoded_len; ++i) {
        printf("%02X ", oer_out[i]);
    }
    printf("\n");
    // Expected: 2A (single byte for value 42, fixed-width big-endian, range 0..255 → 1 octet)

    // ── Verify expected bytes ───────────────────────────────────────────
    if (encoded_len != 1 || oer_out[0] != 0x2A) {
        printf("ERROR: expected bytes [01 2A], got [");
        for (size_t i = 0; i < encoded_len; ++i) {
            printf("%02X ", oer_out[i]);
        }
        printf("]\n");
        return 1;
    }
    printf("PASS: encoded bytes match expected [2A]\n");

    // ── Decode back → verify value == 42 ───────────────────────────────
    buffer_view ibv(oer_out, 2);  // reset view to start
    oer::oer_decoder dec;
    auto dec_result = dec.decode_integer<Int0To255>(ibv);
    if (dec_result.is_err()) {
        printf("ERROR: decode failed with code %d\n", dec_result.error());
        return 1;
    }
    int64_t decoded_value = dec_result.value();
    printf("Decoded value: %d\n", (int)decoded_value);

    if (decoded_value != 42) {
        printf("ERROR: expected decoded value 42, got %d\n", (int)decoded_value);
        return 1;
    }
    printf("PASS: round-trip decode returned 42\n");

    // ── BER vs OER size comparison ───────────────────────────────────────
    // BER (with tag+length prefix):
    //   Tag:   0x02 (INTEGER)
    //   Len:   0x01 (1 byte following)
    //   Value: 0x2A
    //   Total: 3 bytes
    //
    // OER (constrained, fixed-width):
    //   Value: 0x01 0x2A
    //   Total: 2 bytes
    printf("\nSize comparison:\n");
    printf("  BER encoding of 42: 3 bytes (02 01 2A) — tag + length + value\n");
    printf("  OER encoding of 42: 2 bytes (01 2A)   — just the value\n");
    printf("  Savings: 33%% fewer bytes with OER when constraints are known\n");

    printf("\nAll tests passed.\n");
    return 0;
}