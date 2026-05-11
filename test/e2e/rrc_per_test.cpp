#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <cstring>

#include "codec/result.hpp"
#include "codec/per/encoder.hpp"
#include "codec/per/decoder.hpp"
#include "codec/per/uper_encoder.hpp"
#include "codec/per/uper_decoder.hpp"

using namespace asn1pp;
using namespace asn1pp::per;

// ============================================================================
// RRC-like PER Metadata structs
//
// Models a simplified 3GPP RRC message structure:
//
//   RRC-Message ::= SEQUENCE {
//       rrc-TransactionIdentifier    INTEGER (0..3),
//       message                      CHOICE {
//           setup                    Setup,
//           setupComplete            SetupComplete,
//           rrcRelease               NULL
//       }
//   }
//
//   Setup ::= SEQUENCE {
//       transactionId    INTEGER (0..255),
//       payload          OCTET STRING SIZE(1..128)
//   }
//
//   SetupComplete ::= SEQUENCE {
//       transactionId    INTEGER (0..255)
//   }
// ============================================================================

namespace {

#ifndef ASN1PP_EMBEDDED
// INTEGER (0..3) — rrc-TransactionIdentifier, 2 bits
struct rrc_tid_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 3;
    static constexpr bool has_range_constraint = true;
};

// INTEGER (0..255) — generic transaction ID, 8 bits
struct rrc_transaction_id_meta {
    static constexpr int64_t min_value = 0;
    static constexpr int64_t max_value = 255;
    static constexpr bool has_range_constraint = true;
};

// OCTET STRING SIZE(1..128) — payload with size constraint
struct rrc_payload_meta {
    static constexpr size_t min_size = 1;
    static constexpr size_t max_size = 128;
    static constexpr bool has_size_constraint = true;
};

// CHOICE with 3 alternatives { setup(0), setupComplete(1), rrcRelease(2) }
struct rrc_choice_meta {
    static constexpr size_t alternative_count = 3;
};

// Top-level SEQUENCE { tid, choice }
// 2 fields, none optional, no extension
struct rrc_msg_seq_meta {
    static constexpr bool has_extension = false;
};

// Helper: create a mutable buffer filled with zeros
std::vector<uint8_t> make_mutable_buffer(size_t cap) {
    return std::vector<uint8_t>(cap, 0);
}

// Helper: mutable buffer_view (for encoder output)
buffer_view make_view(std::vector<uint8_t>& vec) {
    return buffer_view(vec);
}

// Helper: const buffer_view (for decoder input)
buffer_view make_const_view(const std::vector<uint8_t>& vec) {
    return buffer_view(const_cast<std::vector<uint8_t>&>(vec));
}
#endif // ASN1PP_EMBEDDED

} // namespace

// ============================================================================
// Fixture: RRC PER Round-Trip Test
// ============================================================================

#ifndef ASN1PP_EMBEDDED
class RrcPerRoundTripTest : public ::testing::Test {
protected:
    per_aligned_encoder encoder_;
    per_aligned_decoder decoder_;
};

// ============================================================================
// Test 1: Encode → Decode CHOICE=setup with payload, verify all fields
// ============================================================================

TEST_F(RrcPerRoundTripTest, SetupMessage_RoundTrip_ALIGNED_UT_E2E_RRC_PER_001) {
    // --- Test data ---
    const int64_t orig_tid = 2;             // rrc-TransactionIdentifier = 2
    const int64_t orig_choice = 0;          // CHOICE = setup
    const int64_t orig_transaction_id = 42; // Setup.transactionId = 42
    const uint8_t orig_payload[] = {
        0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE
    };
    const size_t orig_payload_len = 6;

    // --- Encode ---
    auto buf_vec = make_mutable_buffer(128);
    auto view = make_view(buf_vec);

    // Step 1: Start SEQUENCE (extension bit + optional bitmap = 1 bit, aligned → 1 byte)
    auto r_seq = encoder_.encode_sequence_start<rrc_msg_seq_meta>(view, nullptr, 0);
    ASSERT_TRUE(r_seq.is_ok());

    // Step 2: Encode rrc-TransactionIdentifier (2 bits + align → 1 byte)
    auto r_tid = encoder_.encode_integer<rrc_tid_meta>(orig_tid, view);
    ASSERT_TRUE(r_tid.is_ok());

    // Step 3: Encode CHOICE index (2 bits for 3 alternatives + align → 1 byte)
    auto r_choice = encoder_.encode_choice_index<rrc_choice_meta>(orig_choice, view);
    ASSERT_TRUE(r_choice.is_ok());

    // Step 4: Encode Setup.transactionId (8 bits + align → 1 byte)
    auto r_trans = encoder_.encode_integer<rrc_transaction_id_meta>(orig_transaction_id, view);
    ASSERT_TRUE(r_trans.is_ok());

    // Step 5: Encode Setup.payload OCTET STRING SIZE(1..128)
    auto r_payload = encoder_.encode_octet_string<rrc_payload_meta>(
        std::span<const uint8_t>(orig_payload, orig_payload_len), view);
    ASSERT_TRUE(r_payload.is_ok());

    // Step 6: End SEQUENCE (no-op for ALIGNED)
    auto r_end = encoder_.encode_sequence_end(view);
    ASSERT_TRUE(r_end.is_ok());

    // --- Decode ---
    auto dec_view = make_const_view(buf_vec);

    // Step 1: Start SEQUENCE
    auto d_seq = decoder_.decode_sequence_start<rrc_msg_seq_meta>(dec_view, nullptr, 0);
    ASSERT_TRUE(d_seq.is_ok());
    EXPECT_FALSE(d_seq.value());  // extension not chosen

    // Step 2: Decode rrc-TransactionIdentifier
    auto d_tid = decoder_.decode_integer<rrc_tid_meta>(dec_view);
    ASSERT_TRUE(d_tid.is_ok());
    EXPECT_EQ(d_tid.value(), orig_tid);

    // Step 3: Decode CHOICE index
    auto d_choice = decoder_.decode_choice_index<rrc_choice_meta>(dec_view);
    ASSERT_TRUE(d_choice.is_ok());
    EXPECT_EQ(d_choice.value(), orig_choice);

    // Step 4: Decode Setup.transactionId
    auto d_trans = decoder_.decode_integer<rrc_transaction_id_meta>(dec_view);
    ASSERT_TRUE(d_trans.is_ok());
    EXPECT_EQ(d_trans.value(), orig_transaction_id);

    // Step 5: Decode Setup.payload OCTET STRING
    auto d_payload = decoder_.decode_octet_string<rrc_payload_meta>(dec_view);
    ASSERT_TRUE(d_payload.is_ok());
    EXPECT_EQ(d_payload.value().size(), orig_payload_len);
    EXPECT_THAT(d_payload.value(),
                ::testing::ElementsAreArray(orig_payload, orig_payload_len));

    // Step 6: End SEQUENCE
    auto d_end = decoder_.decode_sequence_end(dec_view);
    ASSERT_TRUE(d_end.is_ok());
}

// ============================================================================
// Test 2: CHOICE=setupComplete (no payload), verify all fields
// ============================================================================

TEST_F(RrcPerRoundTripTest, SetupCompleteMessage_RoundTrip_ALIGNED_UT_E2E_RRC_PER_002) {
    const int64_t orig_tid = 1;             // rrc-TransactionIdentifier = 1
    const int64_t orig_choice = 1;          // CHOICE = setupComplete
    const int64_t orig_transaction_id = 200; // SetupComplete.transactionId = 200

    // --- Encode ---
    auto buf_vec = make_mutable_buffer(128);
    auto view = make_view(buf_vec);

    ASSERT_TRUE(encoder_.encode_sequence_start<rrc_msg_seq_meta>(view, nullptr, 0).is_ok());
    ASSERT_TRUE(encoder_.encode_integer<rrc_tid_meta>(orig_tid, view).is_ok());
    ASSERT_TRUE(encoder_.encode_choice_index<rrc_choice_meta>(orig_choice, view).is_ok());
    ASSERT_TRUE(encoder_.encode_integer<rrc_transaction_id_meta>(orig_transaction_id, view).is_ok());
    ASSERT_TRUE(encoder_.encode_sequence_end(view).is_ok());

    // --- Decode ---
    auto dec_view = make_const_view(buf_vec);

    ASSERT_TRUE(decoder_.decode_sequence_start<rrc_msg_seq_meta>(dec_view, nullptr, 0).is_ok());
    auto d_tid = decoder_.decode_integer<rrc_tid_meta>(dec_view);
    ASSERT_TRUE(d_tid.is_ok());
    EXPECT_EQ(d_tid.value(), orig_tid);

    auto d_choice = decoder_.decode_choice_index<rrc_choice_meta>(dec_view);
    ASSERT_TRUE(d_choice.is_ok());
    EXPECT_EQ(d_choice.value(), orig_choice);

    auto d_trans = decoder_.decode_integer<rrc_transaction_id_meta>(dec_view);
    ASSERT_TRUE(d_trans.is_ok());
    EXPECT_EQ(d_trans.value(), orig_transaction_id);

    ASSERT_TRUE(decoder_.decode_sequence_end(dec_view).is_ok());
}

// ============================================================================
// Test 3: CHOICE=rrcRelease (NULL — nothing more to encode)
// ============================================================================

TEST_F(RrcPerRoundTripTest, ReleaseMessage_RoundTrip_ALIGNED_UT_E2E_RRC_PER_003) {
    const int64_t orig_tid = 3;     // rrc-TransactionIdentifier = 3
    const int64_t orig_choice = 2;  // CHOICE = rrcRelease (NULL)

    // --- Encode ---
    auto buf_vec = make_mutable_buffer(128);
    auto view = make_view(buf_vec);

    ASSERT_TRUE(encoder_.encode_sequence_start<rrc_msg_seq_meta>(view, nullptr, 0).is_ok());
    ASSERT_TRUE(encoder_.encode_integer<rrc_tid_meta>(orig_tid, view).is_ok());
    ASSERT_TRUE(encoder_.encode_choice_index<rrc_choice_meta>(orig_choice, view).is_ok());
    ASSERT_TRUE(encoder_.encode_sequence_end(view).is_ok());

    // --- Decode ---
    auto dec_view = make_const_view(buf_vec);

    ASSERT_TRUE(decoder_.decode_sequence_start<rrc_msg_seq_meta>(dec_view, nullptr, 0).is_ok());
    auto d_tid = decoder_.decode_integer<rrc_tid_meta>(dec_view);
    ASSERT_TRUE(d_tid.is_ok());
    EXPECT_EQ(d_tid.value(), orig_tid);

    auto d_choice = decoder_.decode_choice_index<rrc_choice_meta>(dec_view);
    ASSERT_TRUE(d_choice.is_ok());
    EXPECT_EQ(d_choice.value(), orig_choice);

    ASSERT_TRUE(decoder_.decode_sequence_end(dec_view).is_ok());
}

// ============================================================================
// Test 4: Edge values — transaction identifier 0 and 3 (boundary values)
// ============================================================================

TEST_F(RrcPerRoundTripTest, TidBoundaryValues_ALIGNED_UT_E2E_RRC_PER_004) {
    auto buf_vec = make_mutable_buffer(128);
    for (int64_t tid : {0, 3}) {
        buf_vec.assign(buf_vec.size(), 0);
        auto view = make_view(buf_vec);
        encoder_.set_bit_offset(0);

        ASSERT_TRUE(encoder_.encode_sequence_start<rrc_msg_seq_meta>(view, nullptr, 0).is_ok());
        ASSERT_TRUE(encoder_.encode_integer<rrc_tid_meta>(tid, view).is_ok());
        ASSERT_TRUE(encoder_.encode_choice_index<rrc_choice_meta>(2, view).is_ok()); // release
        ASSERT_TRUE(encoder_.encode_sequence_end(view).is_ok());

        auto dec_view = make_const_view(buf_vec);
        decoder_.set_bit_offset(0);

        ASSERT_TRUE(decoder_.decode_sequence_start<rrc_msg_seq_meta>(dec_view, nullptr, 0).is_ok());
        auto d_tid = decoder_.decode_integer<rrc_tid_meta>(dec_view);
        ASSERT_TRUE(d_tid.is_ok());
        EXPECT_EQ(d_tid.value(), tid) << "Failed for tid=" << tid;

        auto d_choice = decoder_.decode_choice_index<rrc_choice_meta>(dec_view);
        ASSERT_TRUE(d_choice.is_ok());
        EXPECT_EQ(d_choice.value(), 2);
        ASSERT_TRUE(decoder_.decode_sequence_end(dec_view).is_ok());
    }
}

// ============================================================================
// Test 5: UPER (unaligned) round-trip with payload
// ============================================================================

TEST_F(RrcPerRoundTripTest, SetupMessage_RoundTrip_UNALIGNED_UT_E2E_RRC_PER_005) {
    const int64_t orig_tid = 2;
    const int64_t orig_choice = 0;          // setup
    const int64_t orig_transaction_id = 99;
    const uint8_t orig_payload[] = {0x01, 0x02, 0x03, 0x04};
    const size_t orig_payload_len = 4;

    // --- UPER Encode ---
    auto enc_buf = make_mutable_buffer(128);
    auto enc_view = make_view(enc_buf);
    uper_encoder uenc;
    uenc.set_bit_offset(0);

    ASSERT_TRUE(uenc.encode_sequence_start<rrc_msg_seq_meta>(enc_view, nullptr, 0).is_ok());
    ASSERT_TRUE(uenc.encode_integer<rrc_tid_meta>(orig_tid, enc_view).is_ok());
    ASSERT_TRUE(uenc.encode_choice_index<rrc_choice_meta>(orig_choice, enc_view).is_ok());
    ASSERT_TRUE(uenc.encode_integer<rrc_transaction_id_meta>(orig_transaction_id, enc_view).is_ok());
    ASSERT_TRUE(uenc.encode_octet_string<rrc_payload_meta>(
        std::span<const uint8_t>(orig_payload, orig_payload_len), enc_view).is_ok());
    ASSERT_TRUE(uenc.encode_sequence_end(enc_view).is_ok());
    uenc.flush(enc_view);

    // --- UPER Decode ---
    auto dec_view = make_view(enc_buf);
    uper_decoder udec;
    udec.set_bit_offset(0);

    ASSERT_TRUE(udec.decode_sequence_start<rrc_msg_seq_meta>(dec_view, nullptr, 0).is_ok());

    auto d_tid = udec.decode_integer<rrc_tid_meta>(dec_view);
    ASSERT_TRUE(d_tid.is_ok());
    EXPECT_EQ(d_tid.value(), orig_tid);

    auto d_choice = udec.decode_choice_index<rrc_choice_meta>(dec_view);
    ASSERT_TRUE(d_choice.is_ok());
    EXPECT_EQ(d_choice.value(), orig_choice);

    auto d_trans = udec.decode_integer<rrc_transaction_id_meta>(dec_view);
    ASSERT_TRUE(d_trans.is_ok());
    EXPECT_EQ(d_trans.value(), orig_transaction_id);

    auto d_payload = udec.decode_octet_string<rrc_payload_meta>(dec_view);
    ASSERT_TRUE(d_payload.is_ok());
    EXPECT_EQ(d_payload.value().size(), orig_payload_len);
    EXPECT_THAT(d_payload.value(),
                ::testing::ElementsAreArray(orig_payload, orig_payload_len));

    ASSERT_TRUE(udec.decode_sequence_end(dec_view).is_ok());
}

// ============================================================================
// Test 6: Different payload sizes — verify size constraint handling
// ============================================================================

TEST_F(RrcPerRoundTripTest, PayloadSizeVariants_ALIGNED_UT_E2E_RRC_PER_006) {
    for (size_t plen : {1, 10, 50, 100}) {
        std::vector<uint8_t> payload(plen);
        for (size_t i = 0; i < plen; ++i) {
            payload[i] = static_cast<uint8_t>(i * 3 + 7);
        }

        auto buf_vec = make_mutable_buffer(1024);
        auto view = make_view(buf_vec);
        encoder_.set_bit_offset(0);

        ASSERT_TRUE(encoder_.encode_sequence_start<rrc_msg_seq_meta>(view, nullptr, 0).is_ok());
        ASSERT_TRUE(encoder_.encode_integer<rrc_tid_meta>(0, view).is_ok());
        ASSERT_TRUE(encoder_.encode_choice_index<rrc_choice_meta>(0, view).is_ok()); // setup
        ASSERT_TRUE(encoder_.encode_integer<rrc_transaction_id_meta>(128, view).is_ok());
        ASSERT_TRUE(encoder_.encode_octet_string<rrc_payload_meta>(
            std::span<const uint8_t>(payload), view).is_ok());
        ASSERT_TRUE(encoder_.encode_sequence_end(view).is_ok());

        auto dec_view = make_const_view(buf_vec);
        decoder_.set_bit_offset(0);

        ASSERT_TRUE(decoder_.decode_sequence_start<rrc_msg_seq_meta>(dec_view, nullptr, 0).is_ok());
        ASSERT_TRUE(decoder_.decode_integer<rrc_tid_meta>(dec_view).is_ok());
        ASSERT_TRUE(decoder_.decode_choice_index<rrc_choice_meta>(dec_view).is_ok());
        ASSERT_TRUE(decoder_.decode_integer<rrc_transaction_id_meta>(dec_view).is_ok());
        auto d_payload = decoder_.decode_octet_string<rrc_payload_meta>(dec_view);
        ASSERT_TRUE(d_payload.is_ok());
        EXPECT_EQ(d_payload.value().size(), plen) << "Failed for plen=" << plen;
        EXPECT_THAT(d_payload.value(), ::testing::ElementsAreArray(payload))
            << "Failed for plen=" << plen;
        ASSERT_TRUE(decoder_.decode_sequence_end(dec_view).is_ok());
    }
}
#endif // ASN1PP_EMBEDDED
