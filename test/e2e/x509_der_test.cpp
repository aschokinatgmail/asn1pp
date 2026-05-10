#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <cstring>
#include <string>
#include <algorithm>

#include "codec/ber/decoder.hpp"
#include "codec/ber/tlv.hpp"
#include "codec/traits.hpp"
#include "codec/result.hpp"
#include "buffer/buffer_view.hpp"

using namespace asn1pp;
using namespace asn1pp::ber;

// ============================================================================
// Test data: minimal realistic DER-encoded X.509 TBSCertificate-like SEQUENCE.
//
// Structure (outer SEQUENCE, 131 content bytes):
//   [0] EXPLICIT version        INTEGER 2 (v3)
//   serialNumber                INTEGER 1
//   signature                   SEQUENCE { OID sha256WithRSAEncryption, NULL }
//   issuer                      SEQUENCE { SET { SEQUENCE { OID commonName, PrintableString } } }
//   validity                    SEQUENCE { UTCTime notBefore, UTCTime notAfter }
//   subject                     SEQUENCE { SET { SEQUENCE { OID commonName, PrintableString } } }
//   subjectPublicKeyInfo        SEQUENCE { SEQUENCE { OID rsaEncryption, NULL }, BIT STRING }
//
// Total: 134 bytes = 3-byte header (30 81 83) + 131 content bytes.
// ============================================================================
namespace {
constexpr uint8_t k_x509_tbs_der[] = {
    // Outer SEQUENCE – 131 content bytes, long-form length
    0x30, 0x81, 0x83,

    // ----- [0] EXPLICIT version : INTEGER 2 (v3 cert) -----------------------
    0xA0, 0x03,                               // context-specific [0], constructed, len 3
    0x02, 0x01, 0x02,                         //   INTEGER 2

    // ----- serialNumber : INTEGER 1 -----------------------------------------
    0x02, 0x01, 0x01,                         // INTEGER 1

    // ----- signatureAlgorithm : SEQUENCE ------------------------------------
    0x30, 0x0D,                               // SEQUENCE (13 content bytes)
    0x06, 0x09,                               //   OID (9 bytes)
    0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B,
                                              //     sha256WithRSAEncryption
    0x05, 0x00,                               //   NULL

    // ----- issuer : SEQUENCE OF SET OF SEQUENCE (DN) ------------------------
    0x30, 0x16,                               // SEQUENCE (22 content bytes)
    0x31, 0x14,                               //   SET (20 content bytes)
    0x30, 0x12,                               //     SEQUENCE (18 content bytes)
    0x06, 0x03, 0x55, 0x04, 0x03,             //       OID commonName (2.5.4.3)
    0x13, 0x0B,                               //       PrintableString (11 bytes)
    0x54, 0x65, 0x73, 0x74, 0x20,             //       "Test CA One"
    0x43, 0x41, 0x20, 0x4F, 0x6E, 0x65,

    // ----- validity : SEQUENCE { UTCTime, UTCTime } -------------------------
    0x30, 0x1E,                               // SEQUENCE (30 content bytes)
    0x17, 0x0D,                               //   UTCTime (13 bytes)
    0x32, 0x35, 0x30, 0x35, 0x31, 0x30,       //   "250510000000Z"
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5A,
    0x17, 0x0D,                               //   UTCTime (13 bytes)
    0x33, 0x35, 0x30, 0x35, 0x31, 0x30,       //   "350510000000Z"
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5A,

    // ----- subject : SEQUENCE OF SET OF SEQUENCE (DN) -----------------------
    0x30, 0x14,                               // SEQUENCE (20 content bytes)
    0x31, 0x12,                               //   SET (18 content bytes)
    0x30, 0x10,                               //     SEQUENCE (16 content bytes)
    0x06, 0x03, 0x55, 0x04, 0x03,             //       OID commonName
    0x13, 0x09,                               //       PrintableString (9 bytes)
    0x6C, 0x6F, 0x63, 0x61, 0x6C,             //       "localhost"
    0x68, 0x6F, 0x73, 0x74,

    // ----- subjectPublicKeyInfo : SEQUENCE ----------------------------------
    0x30, 0x1C,                               // SEQUENCE (28 content bytes)
    0x30, 0x0D,                               //   SEQUENCE – algorithm (13 bytes)
    0x06, 0x09,                               //     OID (9 bytes)
    0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01,
                                              //       rsaEncryption
    0x05, 0x00,                               //     NULL
    0x03, 0x0B,                               //   BIT STRING (11 content bytes)
    0x00,                                     //     unused bits = 0
    0xAA, 0xBB, 0xCC, 0xDD, 0xEE,             //     public key bytes (10 bytes)
    0xFF, 0x01, 0x02, 0x03, 0x04,
};
constexpr size_t k_x509_tbs_size = sizeof(k_x509_tbs_der);

// ── Helpers ────────────────────────────────────────────────────────────

buffer_view make_const_view(const uint8_t* data, size_t size) {
    return buffer_view(data, size);
}

buffer_view make_mutable(std::vector<uint8_t>& vec) {
    return buffer_view(vec);
}

result<void> re_encode_tlv_tree(buffer_view& buf,
                                const ber_decoder::tlv_data& tlv) {
    if (!tlv.t.constructed) {
        // Primitive / leaf — simple TLV re-encode
        return encode_tlv(buf, tlv.t, tlv.value);
    }

    // Constructed type: walk children from the content span.
    // Accumulate re-encoded child bytes, then wrap with outer tag.
    std::vector<uint8_t> children_buf;
    constexpr size_t k_chunk = 256;
    children_buf.reserve(k_chunk);

    buffer_view walk(make_const_view(tlv.value.data(), tlv.value.size()));
    ber_decoder child_dec;

    while (!walk.empty()) {
        auto child_tlv = child_dec.decode_tlv(walk);
        if (child_tlv.is_err()) return result<void>::err(child_tlv.error());

        // Re-encode the child into children_buf
        size_t before = children_buf.size();
        const size_t child_hdr =
            encoded_tag_size(child_tlv.value().t) +
            encoded_length_size(child_tlv.value().value.size());
        children_buf.resize(before + child_hdr + child_tlv.value().value.size());
        buffer_view child_out(children_buf.data() + before, children_buf.size() - before);

        auto enc_r = re_encode_tlv_tree(child_out, child_tlv.value());
        if (enc_r.is_err()) return enc_r;

        size_t written = child_out.data() - (children_buf.data() + before);
        children_buf.resize(before + written);
    }

    // Wrap children in the constructed tag
    return encode_tlv(buf, tlv.t,
                      std::span<const uint8_t>(children_buf.data(), children_buf.size()));
}

result<void> collect_child_tags(buffer_view& walk,
                                std::vector<tag>& out_tags,
                                size_t depth = 0) {
    ber_decoder dec;
    while (!walk.empty()) {
        auto tlv = dec.decode_tlv(walk);
        if (tlv.is_err()) return result<void>::err(tlv.error());

        out_tags.push_back(tlv.value().t);

        if (tlv.value().t.constructed) {
            buffer_view inner(make_const_view(tlv.value().value.data(),
                                              tlv.value().value.size()));
            auto inner_r = collect_child_tags(inner, out_tags, depth + 1);
            if (inner_r.is_err()) return inner_r;
        }
    }
    return result<void>::ok();
}

} // namespace

// ============================================================================
// 1. Decode – structural verification
// ============================================================================

class X509DerDecodeTest : public ::testing::Test {
protected:
    ber_decoder dec_;
};

TEST_F(X509DerDecodeTest, DecodeOuterSequence_E2E_X509_001) {
    buffer_view view = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(view);
    ASSERT_TRUE(outer.is_ok());

    const auto& outer_tlv = outer.value();
    EXPECT_EQ(outer_tlv.t.cls, tag_class::universal);
    EXPECT_TRUE(outer_tlv.t.constructed);
    EXPECT_EQ(outer_tlv.t.number, static_cast<uint32_t>(universal_tag::sequence));
}

TEST_F(X509DerDecodeTest, DecodeAllTopLevelFields_E2E_X509_002) {
    buffer_view view = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(view);
    ASSERT_TRUE(outer.is_ok());

    // Walk content bytes of the outer SEQUENCE — expect 7 top-level TLVs
    buffer_view content = make_const_view(outer.value().value.data(),
                                          outer.value().value.size());
    std::vector<tag> top_tags;

    for (int i = 0; i < 7; ++i) {
        auto child = dec_.decode_tlv(content);
        ASSERT_TRUE(child.is_ok()) << "Failed to decode top-level field " << i;
        top_tags.push_back(child.value().t);
    }

    EXPECT_TRUE(content.empty()) << "Extra bytes after 7 top-level fields";
    EXPECT_EQ(top_tags.size(), 7u);

    // Field 0: context-specific [0] (version wrapper)
    EXPECT_EQ(top_tags[0].cls, tag_class::context_specific);
    EXPECT_EQ(top_tags[0].number, 0u);
    EXPECT_TRUE(top_tags[0].constructed);

    // Field 1: INTEGER (serialNumber)
    EXPECT_EQ(top_tags[1], make_universal(universal_tag::integer));

    // Field 2: SEQUENCE (signatureAlgorithm)
    EXPECT_EQ(top_tags[2], make_universal(universal_tag::sequence, true));

    // Field 3: SEQUENCE (issuer)
    EXPECT_EQ(top_tags[3], make_universal(universal_tag::sequence, true));

    // Field 4: SEQUENCE (validity)
    EXPECT_EQ(top_tags[4], make_universal(universal_tag::sequence, true));

    // Field 5: SEQUENCE (subject)
    EXPECT_EQ(top_tags[5], make_universal(universal_tag::sequence, true));

    // Field 6: SEQUENCE (subjectPublicKeyInfo)
    EXPECT_EQ(top_tags[6], make_universal(universal_tag::sequence, true));
}

TEST_F(X509DerDecodeTest, VerifyVersionField_E2E_X509_003) {
    buffer_view view = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(view);
    ASSERT_TRUE(outer.is_ok());

    buffer_view content = make_const_view(outer.value().value.data(),
                                          outer.value().value.size());

    // Field 0: context-specific [0] wrapping INTEGER
    auto ver_wrapper = dec_.decode_tlv(content);
    ASSERT_TRUE(ver_wrapper.is_ok());
    EXPECT_EQ(ver_wrapper.value().t.cls, tag_class::context_specific);
    EXPECT_EQ(ver_wrapper.value().t.number, 0u);

    // Inside context-specific [0]: INTEGER 2
    buffer_view ver_content = make_const_view(ver_wrapper.value().value.data(),
                                              ver_wrapper.value().value.size());
    auto ver_inner = dec_.decode_tlv(ver_content);
    ASSERT_TRUE(ver_inner.is_ok());
    EXPECT_EQ(ver_inner.value().t, make_universal(universal_tag::integer));
}

TEST_F(X509DerDecodeTest, VerifySerialField_E2E_X509_004) {
    buffer_view view = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(view);
    ASSERT_TRUE(outer.is_ok());

    buffer_view content = make_const_view(outer.value().value.data(),
                                          outer.value().value.size());
    // Skip [0] EXPLICIT
    auto _ = dec_.decode_tlv(content);

    // Field 1: INTEGER
    auto serial = dec_.decode_tlv(content);
    ASSERT_TRUE(serial.is_ok());
    EXPECT_EQ(serial.value().t, make_universal(universal_tag::integer));
    EXPECT_EQ(serial.value().length, 1u);
    EXPECT_EQ(serial.value().value[0], 0x01);
}

TEST_F(X509DerDecodeTest, VerifySignatureAlgorithm_E2E_X509_005) {
    buffer_view view = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(view);
    ASSERT_TRUE(outer.is_ok());

    buffer_view content = make_const_view(outer.value().value.data(),
                                          outer.value().value.size());
    // Skip first 2 fields
    for (int i = 0; i < 2; ++i) { auto _ = dec_.decode_tlv(content); }

    // Field 2: SEQUENCE (signatureAlgorithm)
    auto sig_alg = dec_.decode_tlv(content);
    ASSERT_TRUE(sig_alg.is_ok());
    EXPECT_EQ(sig_alg.value().t, make_universal(universal_tag::sequence, true));

    // Inside: OID + NULL
    buffer_view sig_content = make_const_view(sig_alg.value().value.data(),
                                              sig_alg.value().value.size());
    auto oid_tlv = dec_.decode_tlv(sig_content);
    ASSERT_TRUE(oid_tlv.is_ok());
    EXPECT_EQ(oid_tlv.value().t, make_universal(universal_tag::object_identifier));

    auto null_tlv = dec_.decode_tlv(sig_content);
    ASSERT_TRUE(null_tlv.is_ok());
    EXPECT_EQ(null_tlv.value().t, make_universal(universal_tag::null));
    EXPECT_EQ(null_tlv.value().length, 0u);
}

TEST_F(X509DerDecodeTest, VerifyValidityFields_E2E_X509_006) {
    buffer_view view = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(view);
    ASSERT_TRUE(outer.is_ok());

    buffer_view content = make_const_view(outer.value().value.data(),
                                          outer.value().value.size());
    // Skip first 4 fields
    for (int i = 0; i < 4; ++i) { auto _ = dec_.decode_tlv(content); }

    // Field 4: SEQUENCE (validity)
    auto validity = dec_.decode_tlv(content);
    ASSERT_TRUE(validity.is_ok());
    EXPECT_EQ(validity.value().t, make_universal(universal_tag::sequence, true));

    // Inside: two UTCTime values
    buffer_view val_content = make_const_view(validity.value().value.data(),
                                              validity.value().value.size());
    auto not_before = dec_.decode_tlv(val_content);
    ASSERT_TRUE(not_before.is_ok());
    EXPECT_EQ(not_before.value().t,
              make_universal(universal_tag::utc_time));
    EXPECT_EQ(not_before.value().length, 13u);

    auto not_after = dec_.decode_tlv(val_content);
    ASSERT_TRUE(not_after.is_ok());
    EXPECT_EQ(not_after.value().t,
              make_universal(universal_tag::utc_time));
    EXPECT_EQ(not_after.value().length, 13u);
}

TEST_F(X509DerDecodeTest, VerifySubjectPublicKeyInfo_E2E_X509_007) {
    buffer_view view = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(view);
    ASSERT_TRUE(outer.is_ok());

    buffer_view content = make_const_view(outer.value().value.data(),
                                          outer.value().value.size());
    // Skip first 6 fields
    for (int i = 0; i < 6; ++i) { auto _ = dec_.decode_tlv(content); }

    // Field 6: SEQUENCE (subjectPublicKeyInfo)
    auto spki = dec_.decode_tlv(content);
    ASSERT_TRUE(spki.is_ok());
    EXPECT_EQ(spki.value().t, make_universal(universal_tag::sequence, true));

    // Inside: algorithm SEQUENCE + BIT STRING
    buffer_view spki_content = make_const_view(spki.value().value.data(),
                                               spki.value().value.size());
    auto algo_seq = dec_.decode_tlv(spki_content);
    ASSERT_TRUE(algo_seq.is_ok());
    EXPECT_EQ(algo_seq.value().t, make_universal(universal_tag::sequence, true));

    auto bit_str = dec_.decode_tlv(spki_content);
    ASSERT_TRUE(bit_str.is_ok());
    EXPECT_EQ(bit_str.value().t, make_universal(universal_tag::bit_string));
    EXPECT_GE(bit_str.value().length, 1u);
    // First content byte is unused_bits = 0
    EXPECT_EQ(bit_str.value().value[0], 0x00);
}

TEST_F(X509DerDecodeTest, AllFieldsConsumed_E2E_X509_008) {
    buffer_view view = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(view);
    ASSERT_TRUE(outer.is_ok());
    // View should be fully consumed after reading the whole TLV
    EXPECT_TRUE(view.empty());
}

// ============================================================================
// 2. Re-encode – round-trip fidelity
// ============================================================================

class X509DerRoundtripTest : public ::testing::Test {
protected:
    ber_decoder dec_;
};

TEST_F(X509DerRoundtripTest, EncodeDecodeByteMatch_E2E_X509_009) {
    // Step 1: Decode outer SEQUENCE
    buffer_view src = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(src);
    ASSERT_TRUE(outer.is_ok());

    // Step 2: Re-encode the entire TLV tree into a fresh buffer
    std::vector<uint8_t> reencoded(k_x509_tbs_size * 2 + 64, 0x00);
    buffer_view out = make_mutable(reencoded);

    auto enc_r = re_encode_tlv_tree(out, outer.value());
    ASSERT_TRUE(enc_r.is_ok());

    size_t written = static_cast<size_t>(
        out.data() - reencoded.data());
    ASSERT_EQ(written, k_x509_tbs_size)
        << "Re-encoded size mismatch: got " << written
        << ", expected " << k_x509_tbs_size;

    // Step 3: Byte-for-byte comparison
    std::vector<uint8_t> result_bytes(reencoded.data(), reencoded.data() + written);
    std::vector<uint8_t> original(k_x509_tbs_der, k_x509_tbs_der + k_x509_tbs_size);
    EXPECT_EQ(result_bytes, original);

    // Also verify the re-encoded bytes can be decoded again
    buffer_view round2_view = make_const_view(reencoded.data(), written);
    auto outer2 = dec_.decode_tlv(round2_view);
    ASSERT_TRUE(outer2.is_ok());
    EXPECT_EQ(outer2.value().t, outer.value().t);
    EXPECT_EQ(outer2.value().length, outer.value().length);
    EXPECT_TRUE(round2_view.empty());
}

TEST_F(X509DerRoundtripTest, DecodeReencodeWalkChildren_E2E_X509_010) {
    // Decode, re-encode, then decode again and verify the tag tree matches
    buffer_view src = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(src);
    ASSERT_TRUE(outer.is_ok());

    // Collect all tags from original
    buffer_view orig_content = make_const_view(outer.value().value.data(),
                                               outer.value().value.size());
    std::vector<tag> orig_tags;
    auto col_r = collect_child_tags(orig_content, orig_tags);
    ASSERT_TRUE(col_r.is_ok());

    // Re-encode
    std::vector<uint8_t> buf(k_x509_tbs_size * 2 + 64, 0x00);
    buffer_view out = make_mutable(buf);
    auto enc_r = re_encode_tlv_tree(out, outer.value());
    ASSERT_TRUE(enc_r.is_ok());
    size_t written = static_cast<size_t>(out.data() - buf.data());

    // Decode re-encoded and collect tags
    buffer_view re_src = make_const_view(buf.data(), written);
    auto re_outer = dec_.decode_tlv(re_src);
    ASSERT_TRUE(re_outer.is_ok());

    buffer_view re_content = make_const_view(re_outer.value().value.data(),
                                             re_outer.value().value.size());
    std::vector<tag> re_tags;
    auto re_col_r = collect_child_tags(re_content, re_tags);
    ASSERT_TRUE(re_col_r.is_ok());

    EXPECT_EQ(re_tags.size(), orig_tags.size())
        << "Tag count mismatch after round-trip";

    for (size_t i = 0; i < std::min(orig_tags.size(), re_tags.size()); ++i) {
        EXPECT_EQ(re_tags[i], orig_tags[i])
            << "Tag mismatch at tree position " << i;
    }
}

// ============================================================================
// 3. Determinism – same input → identical encoding every time
// ============================================================================

TEST_F(X509DerRoundtripTest, DeterministicEncoding_E2E_X509_011) {
    buffer_view src = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(src);
    ASSERT_TRUE(outer.is_ok());

    // Encode twice
    auto encode_once = [&]() -> std::vector<uint8_t> {
        std::vector<uint8_t> buf(512, 0x00);
        buffer_view out = make_mutable(buf);
        auto r = re_encode_tlv_tree(out, outer.value());
        if (r.is_err()) return {};
        size_t sz = static_cast<size_t>(out.data() - buf.data());
        return std::vector<uint8_t>(buf.data(), buf.data() + sz);
    };

    auto enc1 = encode_once();
    auto enc2 = encode_once();

    ASSERT_FALSE(enc1.empty());
    ASSERT_FALSE(enc2.empty());
    EXPECT_EQ(enc1, enc2);
}

// ============================================================================
// 4. Idempotent – decode + encode + decode + encode still matches
// ============================================================================

TEST_F(X509DerRoundtripTest, IdempotentDoubleRoundtrip_E2E_X509_012) {
    buffer_view src = make_const_view(k_x509_tbs_der, k_x509_tbs_size);
    auto outer = dec_.decode_tlv(src);
    ASSERT_TRUE(outer.is_ok());

    // Round 1: encode
    std::vector<uint8_t> buf1(512, 0x00);
    buffer_view out1 = make_mutable(buf1);
    auto r1 = re_encode_tlv_tree(out1, outer.value());
    ASSERT_TRUE(r1.is_ok());
    size_t sz1 = static_cast<size_t>(out1.data() - buf1.data());

    // Round 2: decode the round-1 output, then re-encode again
    buffer_view src2 = make_const_view(buf1.data(), sz1);
    auto outer2 = dec_.decode_tlv(src2);
    ASSERT_TRUE(outer2.is_ok());

    std::vector<uint8_t> buf2(512, 0x00);
    buffer_view out2 = make_mutable(buf2);
    auto r2 = re_encode_tlv_tree(out2, outer2.value());
    ASSERT_TRUE(r2.is_ok());
    size_t sz2 = static_cast<size_t>(out2.data() - buf2.data());

    EXPECT_EQ(sz2, sz1);
    EXPECT_EQ(sz2, k_x509_tbs_size);

    std::vector<uint8_t> original(k_x509_tbs_der, k_x509_tbs_der + k_x509_tbs_size);
    std::vector<uint8_t> round2(buf2.data(), buf2.data() + sz2);
    EXPECT_EQ(round2, original);
}
