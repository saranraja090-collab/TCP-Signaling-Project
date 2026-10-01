/**
 * @file unit_tests.c
 * @brief Built-in unit test suite for Sequence Number and Window Size signaling methods.
 */

#include "unit_tests.h"
#include <stdio.h>
#include <string.h>

#include "packet.h"
#include "encoder.h"
#include "error_control.h"
#include "sequence_method.h"
#include "window_method.h"
#include "timing_method.h"
#include "network_simulator.h"
#include "comparison.h"
#include "decoder.h"
#include "verification.h"

static int g_test_failures = 0;

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "[FAIL] Assertion failed: %s at line %d\n", #cond, __LINE__); \
        g_test_failures++; \
        return; \
    } \
} while (0)

/* =========================================================================
 * 1. SEQUENCE SIGNALING UNIT TESTS (Step 2 Requirements)
 * ========================================================================= */

static void test_seq_bit_0_mapping(void) {
    uint32_t inc = sequence_get_increment_for_bit(0, NULL);
    TEST_ASSERT(inc == SEQ_INCREMENT_0);
    TEST_ASSERT(inc == 100);

    uint8_t bit = 0xFF;
    int res = sequence_get_bit_from_delta(100, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 0);

    /* Delta within tolerance (+100 ± 15): test 90, 105, 110, 85, 115 */
    res = sequence_get_bit_from_delta(90, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 0);

    res = sequence_get_bit_from_delta(110, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 0);

    res = sequence_get_bit_from_delta(85, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 0);

    res = sequence_get_bit_from_delta(115, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 0);

    /* Out of tolerance */
    res = sequence_get_bit_from_delta(84, NULL, &bit);
    TEST_ASSERT(res == SEQ_ERR_INVALID_DELTA);

    res = sequence_get_bit_from_delta(116, NULL, &bit);
    TEST_ASSERT(res == SEQ_ERR_INVALID_DELTA);

    res = sequence_get_bit_from_delta(150, NULL, &bit);
    TEST_ASSERT(res == SEQ_ERR_INVALID_DELTA);

    printf("  [PASS] test_seq_bit_0_mapping (Bit 0 -> +100 and tolerances 90, 110, 85, 115 verified)\n");
}

static void test_seq_bit_1_mapping(void) {
    uint32_t inc = sequence_get_increment_for_bit(1, NULL);
    TEST_ASSERT(inc == SEQ_INCREMENT_1);
    TEST_ASSERT(inc == 200);

    uint8_t bit = 0xFF;
    int res = sequence_get_bit_from_delta(200, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 1);

    /* Delta within tolerance (+200 ± 15): test 190, 195, 210, 185, 215 */
    res = sequence_get_bit_from_delta(190, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 1);

    res = sequence_get_bit_from_delta(210, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 1);

    res = sequence_get_bit_from_delta(185, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 1);

    res = sequence_get_bit_from_delta(215, NULL, &bit);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(bit == 1);

    /* Out of tolerance */
    res = sequence_get_bit_from_delta(184, NULL, &bit);
    TEST_ASSERT(res == SEQ_ERR_INVALID_DELTA);

    res = sequence_get_bit_from_delta(216, NULL, &bit);
    TEST_ASSERT(res == SEQ_ERR_INVALID_DELTA);

    printf("  [PASS] test_seq_bit_1_mapping (Bit 1 -> +200 and tolerances 190, 210, 185, 215 verified)\n");
}

static void test_seq_number_generation(void) {
    uint8_t bits[] = {0, 1, 0, 1};
    simulated_packet_t pkts[8];
    size_t created = 0;

    int res = sequence_method_modulate(bits, 4, NULL, pkts, 8, &created);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(created == 4);

    /* Base is 10000 */
    TEST_ASSERT(pkts[0].sequence_number == 10000 + 100); /* 10100 */
    TEST_ASSERT(pkts[1].sequence_number == 10100 + 200); /* 10300 */
    TEST_ASSERT(pkts[2].sequence_number == 10300 + 100); /* 10400 */
    TEST_ASSERT(pkts[3].sequence_number == 10400 + 200); /* 10600 */

    /* Packet metadata checks */
    TEST_ASSERT(pkts[0].packet_id == 0);
    TEST_ASSERT(pkts[1].packet_id == 1);
    TEST_ASSERT(pkts[0].signaling_method == SIGNALING_METHOD_SEQUENCE);
    printf("  [PASS] test_seq_number_generation (Monotonic increments 10100, 10300, 10400, 10600 verified)\n");
}

static void test_seq_demodulation(void) {
    simulated_packet_t pkts[4];
    for (size_t i = 0; i < 4; i++) packet_init(&pkts[i], (uint32_t)i);

    /* Emulate simulated packet sequence with base 10000:
     * Pkt 0: 10100 (delta +100 -> bit 0)
     * Pkt 1: 10300 (delta +200 -> bit 1)
     * Pkt 2: 10400 (delta +100 -> bit 0)
     * Pkt 3: 10600 (delta +200 -> bit 1)
     */
    pkts[0].sequence_number = 10100;
    pkts[1].sequence_number = 10300;
    pkts[2].sequence_number = 10400;
    pkts[3].sequence_number = 10600;

    uint8_t rec_bits[8];
    size_t rec_count = 0;
    size_t invalid_idx = 0;

    int res = sequence_method_demodulate(pkts, 4, NULL, rec_bits, 8, &rec_count, &invalid_idx);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(rec_count == 4);
    TEST_ASSERT(rec_bits[0] == 0);
    TEST_ASSERT(rec_bits[1] == 1);
    TEST_ASSERT(rec_bits[2] == 0);
    TEST_ASSERT(rec_bits[3] == 1);
    printf("  [PASS] test_seq_demodulation (Demodulated [0, 1, 0, 1] from packet stream)\n");
}

static void test_seq_payload_zero_invariant(void) {
    uint8_t bits[64];
    for (size_t i = 0; i < 64; i++) bits[i] = (uint8_t)(i % 2);

    simulated_packet_t pkts[64];
    size_t created = 0;

    int res = sequence_method_modulate(bits, 64, NULL, pkts, 64, &created);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(created == 64);

    /* CRITICAL INVARIANT: check every single packet */
    for (size_t i = 0; i < 64; i++) {
        TEST_ASSERT(pkts[i].payload_length == 0);
    }
    printf("  [PASS] test_seq_payload_zero_invariant (Strict 0-byte payload on all 64 packets)\n");
}

static void test_seq_hello_round_trip(void) {
    const char *orig = "HELLO";
    uint8_t bits[64];
    size_t num_bits = 0;

    /* 1. Encode text to bits */
    int res = encoder_text_to_bits(orig, bits, 64, &num_bits);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(num_bits == 40); /* 5 chars * 8 bits */

    /* 2. Modulate into simulated packets */
    simulated_packet_t pkts[64];
    size_t created = 0;
    res = sequence_method_modulate(bits, num_bits, NULL, pkts, 64, &created);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(created == 40);

    /* 3. Demodulate packets back to bits */
    uint8_t rec_bits[64];
    size_t recovered = 0;
    size_t invalid_idx = 0;
    res = sequence_method_demodulate(pkts, created, NULL, rec_bits, 64, &recovered, &invalid_idx);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(recovered == 40);
    TEST_ASSERT(memcmp(bits, rec_bits, 40) == 0);

    /* 4. Decode bits to text */
    char decoded[32];
    size_t chars = 0;
    res = decoder_bits_to_text(rec_bits, recovered, decoded, 32, &chars);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(chars == 5);
    TEST_ASSERT(strcmp(orig, decoded) == 0);
    TEST_ASSERT(verification_check_match(orig, decoded) == 1);
    printf("  [PASS] test_seq_hello_round_trip (HELLO: 40 packets, exact match)\n");
}

static void test_seq_longer_message_round_trip(void) {
    const char *orig = "TCP Header Data Modulation & Signaling: Computer Networks 2026 Academic Research Project!";
    size_t orig_len = strlen(orig);
    size_t expected_bits = orig_len * 8;

    uint8_t bits[1024];
    size_t num_bits = 0;
    TEST_ASSERT(encoder_text_to_bits(orig, bits, 1024, &num_bits) == 0);
    TEST_ASSERT(num_bits == expected_bits);

    simulated_packet_t pkts[1024];
    size_t created = 0;
    TEST_ASSERT(sequence_method_modulate(bits, num_bits, NULL, pkts, 1024, &created) == SEQ_SUCCESS);
    TEST_ASSERT(created == expected_bits);

    /* Verify payload invariant */
    for (size_t i = 0; i < created; i++) {
        TEST_ASSERT(pkts[i].payload_length == 0);
    }

    uint8_t rec_bits[1024];
    size_t recovered = 0;
    size_t invalid_idx = 0;
    TEST_ASSERT(sequence_method_demodulate(pkts, created, NULL, rec_bits, 1024, &recovered, &invalid_idx) == SEQ_SUCCESS);
    TEST_ASSERT(recovered == expected_bits);

    char decoded[256];
    size_t chars = 0;
    TEST_ASSERT(decoder_bits_to_text(rec_bits, recovered, decoded, 256, &chars) == 0);
    TEST_ASSERT(chars == orig_len);
    TEST_ASSERT(strcmp(orig, decoded) == 0);
    TEST_ASSERT(verification_check_match(orig, decoded) == 1);
    printf("  [PASS] test_seq_longer_message_round_trip (%u bytes / %u packets exact roundtrip)\n",
           (unsigned int)orig_len, (unsigned int)expected_bits);
}

static void test_seq_empty_input_handling(void) {
    simulated_packet_t pkts[8];
    size_t created = 99;

    /* Empty modulation */
    int res = sequence_method_modulate(NULL, 0, NULL, pkts, 8, &created);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(created == 0);

    /* Empty demodulation */
    uint8_t rec_bits[8];
    size_t recovered = 99;
    size_t invalid_idx = 0;
    res = sequence_method_demodulate(NULL, 0, NULL, rec_bits, 8, &recovered, &invalid_idx);
    TEST_ASSERT(res == SEQ_SUCCESS);
    TEST_ASSERT(recovered == 0);
    printf("  [PASS] test_seq_empty_input_handling (Handled 0-length input gracefully)\n");
}

static void test_seq_invalid_delta_handling(void) {
    simulated_packet_t pkts[3];
    for (size_t i = 0; i < 3; i++) packet_init(&pkts[i], (uint32_t)i);

    /* Base: 10000
     * Pkt 0: 10100 (delta +100 -> valid bit 0)
     * Pkt 1: 10150 (delta +50 -> INVALID DELTA!)
     * Pkt 2: 10350 (delta +200 -> valid bit 1)
     */
    pkts[0].sequence_number = 10100;
    pkts[1].sequence_number = 10150;
    pkts[2].sequence_number = 10350;

    uint8_t rec_bits[8];
    size_t recovered = 0;
    size_t invalid_idx = 999;

    int res = sequence_method_demodulate(pkts, 3, NULL, rec_bits, 8, &recovered, &invalid_idx);
    TEST_ASSERT(res == SEQ_ERR_INVALID_DELTA);
    TEST_ASSERT(invalid_idx == 1); /* Pkt 1 was invalid */
    printf("  [PASS] test_seq_invalid_delta_handling (Correctly detected invalid delta at packet #1)\n");
}

/* =========================================================================
 * 2. WINDOW SIZE SIGNALING UNIT TESTS (Step 3 Requirements)
 * ========================================================================= */

/* Test 1: Bit 0 -> WINDOW_VALUE_0 */
static void test_win_bit_0_mapping(void) {
    uint16_t w = window_get_size_for_bit(0, NULL);
    TEST_ASSERT(w == WINDOW_VALUE_0);
    TEST_ASSERT(w == 30000U);

    uint8_t bit = 0xFF;
    int res = window_get_bit_from_size(30000U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 0);

    /* Within tolerance (±500): test boundaries 29500 and 30500 */
    res = window_get_bit_from_size(30150U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 0);

    res = window_get_bit_from_size(29850U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 0);

    res = window_get_bit_from_size(29500U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 0);

    res = window_get_bit_from_size(30500U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 0);

    /* Just outside boundaries */
    res = window_get_bit_from_size(29499U, NULL, &bit);
    TEST_ASSERT(res == WIN_ERR_INVALID_WINDOW);

    res = window_get_bit_from_size(30501U, NULL, &bit);
    TEST_ASSERT(res == WIN_ERR_INVALID_WINDOW);

    /* Intermediate & extreme values */
    res = window_get_bit_from_size(45000U, NULL, &bit);
    TEST_ASSERT(res == WIN_ERR_INVALID_WINDOW);

    res = window_get_bit_from_size(10000U, NULL, &bit);
    TEST_ASSERT(res == WIN_ERR_INVALID_WINDOW);

    printf("  [PASS] test_win_bit_0_mapping (Bit 0 -> 30000 and boundaries 29500..30500 verified)\n");
}

/* Test 2: Bit 1 -> WINDOW_VALUE_1 */
static void test_win_bit_1_mapping(void) {
    uint16_t w = window_get_size_for_bit(1, NULL);
    TEST_ASSERT(w == WINDOW_VALUE_1);
    TEST_ASSERT(w == 60000U);

    uint8_t bit = 0xFF;
    int res = window_get_bit_from_size(60000U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 1);

    /* Within tolerance (±500): test boundaries 59500 and 60500 */
    res = window_get_bit_from_size(60200U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 1);

    res = window_get_bit_from_size(59800U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 1);

    res = window_get_bit_from_size(59500U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 1);

    res = window_get_bit_from_size(60500U, NULL, &bit);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(bit == 1);

    /* Just outside boundaries */
    res = window_get_bit_from_size(59499U, NULL, &bit);
    TEST_ASSERT(res == WIN_ERR_INVALID_WINDOW);

    res = window_get_bit_from_size(60501U, NULL, &bit);
    TEST_ASSERT(res == WIN_ERR_INVALID_WINDOW);

    /* Out of range */
    res = window_get_bit_from_size(65535U, NULL, &bit);
    TEST_ASSERT(res == WIN_ERR_INVALID_WINDOW);

    printf("  [PASS] test_win_bit_1_mapping (Bit 1 -> 60000 and boundaries 59500..60500 verified)\n");
}

/* Test 3: Window modulation */
static void test_win_modulation(void) {
    uint8_t bits[] = {0, 1, 1, 0, 1};
    simulated_packet_t pkts[8];
    size_t created = 0;

    int res = window_method_modulate(bits, 5, NULL, pkts, 8, &created);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(created == 5);

    /* Verify modulated window values */
    TEST_ASSERT(pkts[0].window_size == 30000U);
    TEST_ASSERT(pkts[1].window_size == 60000U);
    TEST_ASSERT(pkts[2].window_size == 60000U);
    TEST_ASSERT(pkts[3].window_size == 30000U);
    TEST_ASSERT(pkts[4].window_size == 60000U);

    /* Packet fields: packet_id, sequence_number, acknowledgment_number, etc. */
    for (size_t i = 0; i < created; i++) {
        TEST_ASSERT(pkts[i].packet_id == (uint32_t)i);
        TEST_ASSERT(pkts[i].signaling_method == SIGNALING_METHOD_WINDOW);
        TEST_ASSERT(pkts[i].signaled_symbol == bits[i]);
        TEST_ASSERT(pkts[i].acknowledgment_number == 1);
        TEST_ASSERT(pkts[i].flags == TCP_FLAG_ACK);
        TEST_ASSERT(pkts[i].payload_length == 0);
    }
    printf("  [PASS] test_win_modulation (Packet fields & window modulation verified)\n");
}

/* Test 4: Window demodulation */
static void test_win_demodulation(void) {
    simulated_packet_t pkts[4];
    for (size_t i = 0; i < 4; i++) packet_init(&pkts[i], (uint32_t)i);

    pkts[0].window_size = 30000U; /* bit 0 */
    pkts[1].window_size = 60000U; /* bit 1 */
    pkts[2].window_size = 60000U; /* bit 1 */
    pkts[3].window_size = 30000U; /* bit 0 */

    uint8_t rec_bits[8];
    size_t rec_count = 0;
    size_t invalid_idx = 0;

    int res = window_method_demodulate(pkts, 4, NULL, rec_bits, 8, &rec_count, &invalid_idx);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(rec_count == 4);
    TEST_ASSERT(rec_bits[0] == 0);
    TEST_ASSERT(rec_bits[1] == 1);
    TEST_ASSERT(rec_bits[2] == 1);
    TEST_ASSERT(rec_bits[3] == 0);
    printf("  [PASS] test_win_demodulation (Demodulated [0, 1, 1, 0] from packet window sizes)\n");
}

/* Test 5: Payload always 0 */
static void test_win_payload_always_zero(void) {
    uint8_t bits[100];
    for (size_t i = 0; i < 100; i++) bits[i] = (uint8_t)(i % 2);

    simulated_packet_t pkts[100];
    size_t created = 0;

    int res = window_method_modulate(bits, 100, NULL, pkts, 100, &created);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(created == 100);

    /* CRITICAL INVARIANT: payload must be strictly 0 bytes on all packets */
    for (size_t i = 0; i < 100; i++) {
        TEST_ASSERT(pkts[i].payload_length == 0);
    }
    printf("  [PASS] test_win_payload_always_zero (Strict 0-byte payload invariant on all 100 packets)\n");
}

/* Test 6: HELLO round trip */
static void test_win_hello_round_trip(void) {
    const char *orig = "HELLO";
    uint8_t bits[64];
    size_t num_bits = 0;

    /* 1. Encode */
    int res = encoder_text_to_bits(orig, bits, 64, &num_bits);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(num_bits == 40); /* 5 chars * 8 bits */

    /* 2. Modulate */
    simulated_packet_t pkts[64];
    size_t created = 0;
    res = window_method_modulate(bits, num_bits, NULL, pkts, 64, &created);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(created == 40);

    for (size_t i = 0; i < created; i++) {
        TEST_ASSERT(pkts[i].payload_length == 0);
    }

    /* 3. Demodulate */
    uint8_t rec_bits[64];
    size_t recovered = 0;
    size_t invalid_idx = 0;
    res = window_method_demodulate(pkts, created, NULL, rec_bits, 64, &recovered, &invalid_idx);
    TEST_ASSERT(res == WIN_SUCCESS);
    TEST_ASSERT(recovered == 40);
    TEST_ASSERT(memcmp(bits, rec_bits, 40) == 0);

    /* 4. Decode */
    char decoded[32];
    size_t chars = 0;
    res = decoder_bits_to_text(rec_bits, recovered, decoded, 32, &chars);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(chars == 5);
    TEST_ASSERT(strcmp(orig, decoded) == 0);
    TEST_ASSERT(verification_check_match(orig, decoded) == 1);
    printf("  [PASS] test_win_hello_round_trip (HELLO: 40 packets, 0 payload, exact match)\n");
}

/* Test 7: HELLO NETWORK round trip */
static void test_win_hello_network_round_trip(void) {
    const char *orig = "HELLO NETWORK";
    size_t orig_len = strlen(orig); /* 13 chars */
    size_t expected_bits = orig_len * 8; /* 104 bits */

    uint8_t bits[256];
    size_t num_bits = 0;
    TEST_ASSERT(encoder_text_to_bits(orig, bits, 256, &num_bits) == 0);
    TEST_ASSERT(num_bits == expected_bits);

    simulated_packet_t pkts[256];
    size_t created = 0;
    TEST_ASSERT(window_method_modulate(bits, num_bits, NULL, pkts, 256, &created) == WIN_SUCCESS);
    TEST_ASSERT(created == expected_bits);

    for (size_t i = 0; i < created; i++) {
        TEST_ASSERT(pkts[i].payload_length == 0);
    }

    uint8_t rec_bits[256];
    size_t recovered = 0;
    size_t invalid_idx = 0;
    TEST_ASSERT(window_method_demodulate(pkts, created, NULL, rec_bits, 256, &recovered, &invalid_idx) == WIN_SUCCESS);
    TEST_ASSERT(recovered == expected_bits);
    TEST_ASSERT(memcmp(bits, rec_bits, expected_bits) == 0);

    char decoded[64];
    size_t chars = 0;
    TEST_ASSERT(decoder_bits_to_text(rec_bits, recovered, decoded, 64, &chars) == 0);
    TEST_ASSERT(chars == 13);
    TEST_ASSERT(strcmp(orig, decoded) == 0);
    TEST_ASSERT(verification_check_match(orig, decoded) == 1);
    printf("  [PASS] test_win_hello_network_round_trip (HELLO NETWORK: 104 packets, 0 payload, exact match)\n");
}

/* Test 8: Longer message round trip */
static void test_win_longer_message_round_trip(void) {
    const char *orig = "Window Size Signaling modulates flow control advertised window sizes (30000 vs 60000) with strictly 0 bytes payload!";
    size_t orig_len = strlen(orig);
    size_t expected_bits = orig_len * 8;

    uint8_t bits[2048];
    size_t num_bits = 0;
    TEST_ASSERT(encoder_text_to_bits(orig, bits, 2048, &num_bits) == 0);
    TEST_ASSERT(num_bits == expected_bits);

    simulated_packet_t pkts[2048];
    size_t created = 0;
    TEST_ASSERT(window_method_modulate(bits, num_bits, NULL, pkts, 2048, &created) == WIN_SUCCESS);
    TEST_ASSERT(created == expected_bits);

    for (size_t i = 0; i < created; i++) {
        TEST_ASSERT(pkts[i].payload_length == 0);
    }

    uint8_t rec_bits[2048];
    size_t recovered = 0;
    size_t invalid_idx = 0;
    TEST_ASSERT(window_method_demodulate(pkts, created, NULL, rec_bits, 2048, &recovered, &invalid_idx) == WIN_SUCCESS);
    TEST_ASSERT(recovered == expected_bits);

    char decoded[512];
    size_t chars = 0;
    TEST_ASSERT(decoder_bits_to_text(rec_bits, recovered, decoded, 512, &chars) == 0);
    TEST_ASSERT(chars == orig_len);
    TEST_ASSERT(strcmp(orig, decoded) == 0);
    TEST_ASSERT(verification_check_match(orig, decoded) == 1);
    printf("  [PASS] test_win_longer_message_round_trip (%u bytes / %u packets exact roundtrip)\n",
           (unsigned int)orig_len, (unsigned int)expected_bits);
}

/* Test 9: Invalid window value detection */
static void test_win_invalid_window_detection(void) {
    simulated_packet_t pkts[4];
    for (size_t i = 0; i < 4; i++) packet_init(&pkts[i], (uint32_t)i);

    pkts[0].window_size = 30000U; /* valid bit 0 */
    pkts[1].window_size = 45000U; /* INVALID WINDOW SIZE */
    pkts[2].window_size = 60000U; /* valid bit 1 */
    pkts[3].window_size = 30000U; /* valid bit 0 */

    uint8_t rec_bits[8];
    size_t recovered = 0;
    size_t invalid_idx = 999;

    int res = window_method_demodulate(pkts, 4, NULL, rec_bits, 8, &recovered, &invalid_idx);
    TEST_ASSERT(res == WIN_ERR_INVALID_WINDOW);
    TEST_ASSERT(invalid_idx == 1); /* Identified packet index 1 */

    /* Also verify individual getter */
    uint8_t bit = 0xFF;
    TEST_ASSERT(window_get_bit_from_size(45000U, NULL, &bit) == WIN_ERR_INVALID_WINDOW);
    TEST_ASSERT(window_get_bit_from_size(12345U, NULL, &bit) == WIN_ERR_INVALID_WINDOW);
    TEST_ASSERT(window_get_bit_from_size(0, NULL, &bit) == WIN_ERR_INVALID_WINDOW);

    printf("  [PASS] test_win_invalid_window_detection (Correctly rejected invalid window 45000 at index 1)\n");
}

/* =========================================================================
 * 3. OTHER ARCHITECTURAL & COMPONENT TESTS
 * ========================================================================= */

static void test_packet_structure(void) {
    simulated_packet_t pkt;
    packet_init(&pkt, 42);

    TEST_ASSERT(pkt.packet_id == 42);
    TEST_ASSERT(pkt.payload_length == 0);
    TEST_ASSERT(pkt.sequence_number == 10000);
    TEST_ASSERT(pkt.window_size == 65535);
    TEST_ASSERT(pkt.flags == TCP_FLAG_ACK);
    printf("  [PASS] test_packet_structure (Packet model defaults verified)\n");
}

static void test_encoder_decoder(void) {
    const char *orig = "HELLO NETWORKS";
    uint8_t bits[1024];
    size_t num_bits = 0;

    int res = encoder_text_to_bits(orig, bits, 1024, &num_bits);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(num_bits == strlen(orig) * 8);

    char decoded[256];
    size_t chars = 0;
    res = decoder_bits_to_text(bits, num_bits, decoded, 256, &chars);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(chars == strlen(orig));
    TEST_ASSERT(strcmp(orig, decoded) == 0);
    TEST_ASSERT(verification_check_match(orig, decoded) == 1);
    printf("  [PASS] test_encoder_decoder (Roundtrip text/bitstream translation)\n");
}

static void test_timing_signaling_placeholder(void) {
    uint8_t bits[] = {1, 1, 0, 0, 1, 0, 1, 0};
    size_t n = sizeof(bits) / sizeof(bits[0]);
    simulated_packet_t pkts[16];
    size_t pkts_created = 0;

    int res = timing_method_modulate(bits, n, NULL, pkts, 16, &pkts_created);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(pkts_created == n);

    for (size_t i = 0; i < pkts_created; i++) {
        TEST_ASSERT(pkts[i].payload_length == 0);
        TEST_ASSERT(pkts[i].signaling_method == SIGNALING_METHOD_TIMING);
    }

    uint8_t recovered[16];
    size_t rec_count = 0;
    res = timing_method_demodulate(pkts, pkts_created, NULL, recovered, 16, &rec_count);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(rec_count == n);
    TEST_ASSERT(memcmp(bits, recovered, n) == 0);
    printf("  [PASS] test_timing_signaling_placeholder (Timing placeholder signaling verified)\n");
}

static void test_error_control(void) {
    const uint8_t data[] = "NET";
    uint16_t crc = error_control_crc16(data, 3);
    TEST_ASSERT(crc != 0);
    TEST_ASSERT(error_control_verify_crc16(data, 3, crc) == 1);
    TEST_ASSERT(error_control_verify_crc16(data, 3, crc ^ 0x01) == 0);

    uint8_t nibble = 0xB; /* 1011 */
    uint8_t codeword = error_control_hamming_encode_nibble(nibble);

    int corrected = 0, error_pos = 0;
    uint8_t decoded = error_control_hamming_decode_nibble(codeword, &corrected, &error_pos);
    TEST_ASSERT(decoded == nibble);
    TEST_ASSERT(corrected == 0);
    TEST_ASSERT(error_pos == 0);

    /* Corrupt 1 bit at position 5 */
    uint8_t corrupted = codeword ^ (1 << 4);
    decoded = error_control_hamming_decode_nibble(corrupted, &corrected, &error_pos);
    TEST_ASSERT(decoded == nibble);
    TEST_ASSERT(corrected == 1);
    TEST_ASSERT(error_pos == 5);
    printf("  [PASS] test_error_control (CRC-16 & Hamming(7,4) correction verified)\n");
}

/* =========================================================================
 * 4. NETWORK IMPAIRMENT & SIMULATION TESTS (Step 4 Requirements)
 * ========================================================================= */

/* Test 1: Normal network transmission (baseline) */
static void test_net_normal_transmission(void) {
    simulated_packet_t tx[16];
    for (size_t i = 0; i < 16; i++) {
        packet_init(&tx[i], (uint32_t)i);
        tx[i].sequence_number = 10000 + (uint32_t)(i * 100);
        tx[i].window_size = (i % 2 == 0) ? 30000 : 60000;
        tx[i].payload_length = 0;
    }

    network_config_t cfg;
    network_config_init_default(&cfg);

    simulated_packet_t rx[16];
    size_t rx_count = 0;
    network_stats_t stats;
    int res = network_simulator_transmit(tx, 16, &cfg, rx, 16, &rx_count, &stats, NULL, 0, NULL);

    TEST_ASSERT(res == 0);
    TEST_ASSERT(rx_count == 16);
    TEST_ASSERT(stats.packets_generated == 16);
    TEST_ASSERT(stats.packets_received == 16);
    TEST_ASSERT(stats.packets_lost == 0);
    TEST_ASSERT(stats.packets_corrupted == 0);
    TEST_ASSERT(stats.packets_reordered == 0);

    for (size_t i = 0; i < 16; i++) {
        TEST_ASSERT(rx[i].packet_id == tx[i].packet_id);
        TEST_ASSERT(rx[i].sequence_number == tx[i].sequence_number);
        TEST_ASSERT(rx[i].window_size == tx[i].window_size);
        TEST_ASSERT(rx[i].payload_length == 0);
    }
    printf("  [PASS] test_net_normal_transmission (16 packets forwarded intact with 0 B payload)\n");
}

/* Test 2: Zero packet loss */
static void test_net_zero_packet_loss(void) {
    simulated_packet_t tx[32];
    for (size_t i = 0; i < 32; i++) {
        packet_init(&tx[i], (uint32_t)i);
        tx[i].payload_length = 0;
    }

    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.loss_percent = 0;
    cfg.enable_loss = 1;

    simulated_packet_t rx[32];
    size_t rx_count = 0;
    network_stats_t stats;
    int res = network_simulator_transmit(tx, 32, &cfg, rx, 32, &rx_count, &stats, NULL, 0, NULL);

    TEST_ASSERT(res == 0);
    TEST_ASSERT(rx_count == 32);
    TEST_ASSERT(stats.packets_lost == 0);
    printf("  [PASS] test_net_zero_packet_loss (0%% loss rate preserved all 32 packets)\n");
}

/* Test 3: Packet loss simulation */
static void test_net_packet_loss(void) {
    simulated_packet_t tx[100];
    for (size_t i = 0; i < 100; i++) {
        packet_init(&tx[i], (uint32_t)i);
        tx[i].payload_length = 0;
    }

    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.loss_percent = 25;
    cfg.enable_loss = 1;
    cfg.random_seed = 42U;

    simulated_packet_t rx[100];
    size_t rx_count = 0;
    network_stats_t stats;
    packet_flow_record_t flow[100];
    size_t flow_count = 0;

    int res = network_simulator_transmit(tx, 100, &cfg, rx, 100, &rx_count, &stats, flow, 100, &flow_count);

    TEST_ASSERT(res == 0);
    TEST_ASSERT(stats.packets_generated == 100);
    TEST_ASSERT(stats.packets_lost > 0);
    TEST_ASSERT(stats.packets_received < 100);
    TEST_ASSERT(stats.packets_lost + stats.packets_received == 100);
    TEST_ASSERT(rx_count == stats.packets_received);

    int found_lost_flow = 0;
    for (size_t i = 0; i < flow_count; i++) {
        if (flow[i].status == PACKET_STATUS_LOST) found_lost_flow = 1;
    }
    TEST_ASSERT(found_lost_flow == 1);
    printf("  [PASS] test_net_packet_loss (Lost %u packets out of 100 correctly tracked)\n", (unsigned int)stats.packets_lost);
}

/* Test 4: Packet corruption simulation */
static void test_net_packet_corruption(void) {
    simulated_packet_t tx[50];
    for (size_t i = 0; i < 50; i++) {
        packet_init(&tx[i], (uint32_t)i);
        tx[i].signaling_method = (i % 2 == 0) ? SIGNALING_METHOD_SEQUENCE : SIGNALING_METHOD_WINDOW;
        tx[i].sequence_number = 10000 + (uint32_t)(i * 100);
        tx[i].window_size = 30000;
        tx[i].payload_length = 0;
    }

    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.corruption_percent = 30;
    cfg.enable_corruption = 1;
    cfg.random_seed = 12345U;

    simulated_packet_t rx[50];
    size_t rx_count = 0;
    network_stats_t stats;

    int res = network_simulator_transmit(tx, 50, &cfg, rx, 50, &rx_count, &stats, NULL, 0, NULL);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(stats.packets_corrupted > 0);
    TEST_ASSERT(rx_count == 50);

    for (size_t i = 0; i < rx_count; i++) {
        TEST_ASSERT(rx[i].payload_length == 0);
    }
    printf("  [PASS] test_net_packet_corruption (%u packets corrupted, 0 B payload preserved)\n", (unsigned int)stats.packets_corrupted);
}

/* Test 5: Sequence corruption detection by receiver */
static void test_net_sequence_corruption_detection(void) {
    const char *msg = "HELLO NETWORK";
    uint8_t bits[256];
    size_t num_bits = 0;
    encoder_text_to_bits(msg, bits, 256, &num_bits);

    simulated_packet_t tx[256];
    size_t tx_count = 0;
    sequence_method_modulate(bits, num_bits, NULL, tx, 256, &tx_count);

    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.corruption_percent = 20;
    cfg.enable_corruption = 1;
    cfg.random_seed = 999U;

    simulated_packet_t rx[256];
    size_t rx_count = 0;
    network_stats_t stats;
    network_simulator_transmit(tx, tx_count, &cfg, rx, 256, &rx_count, &stats, NULL, 0, NULL);

    TEST_ASSERT(stats.packets_corrupted > 0);

    uint8_t rec_bits[256];
    size_t rec_count = 0;
    size_t invalid_idx = 0;
    int dem_res = sequence_method_demodulate(rx, rx_count, NULL, rec_bits, 256, &rec_count, &invalid_idx);

    TEST_ASSERT(dem_res == SEQ_ERR_INVALID_DELTA);
    printf("  [PASS] test_net_sequence_corruption_detection (Receiver detected INVALID_SEQUENCE_DELTA)\n");
}

/* Test 6: Window corruption detection by receiver */
static void test_net_window_corruption_detection(void) {
    const char *msg = "HELLO NETWORK";
    uint8_t bits[256];
    size_t num_bits = 0;
    encoder_text_to_bits(msg, bits, 256, &num_bits);

    simulated_packet_t tx[256];
    size_t tx_count = 0;
    window_method_modulate(bits, num_bits, NULL, tx, 256, &tx_count);

    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.corruption_percent = 20;
    cfg.enable_corruption = 1;
    cfg.random_seed = 999U;

    simulated_packet_t rx[256];
    size_t rx_count = 0;
    network_stats_t stats;
    network_simulator_transmit(tx, tx_count, &cfg, rx, 256, &rx_count, &stats, NULL, 0, NULL);

    TEST_ASSERT(stats.packets_corrupted > 0);

    uint8_t rec_bits[256];
    size_t rec_count = 0;
    size_t invalid_idx = 0;
    int dem_res = window_method_demodulate(rx, rx_count, NULL, rec_bits, 256, &rec_count, &invalid_idx);

    TEST_ASSERT(dem_res == WIN_ERR_INVALID_WINDOW);
    printf("  [PASS] test_net_window_corruption_detection (Receiver detected INVALID_WINDOW_VALUE)\n");
}

/* Test 7: Packet reordering simulation */
static void test_net_packet_reordering(void) {
    simulated_packet_t tx[50];
    for (size_t i = 0; i < 50; i++) {
        packet_init(&tx[i], (uint32_t)i);
        tx[i].sequence_number = 10000 + (uint32_t)(i * 100);
        tx[i].payload_length = 0;
    }

    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.reorder_percent = 40;
    cfg.enable_reordering = 1;
    cfg.random_seed = 54321U;

    simulated_packet_t rx[50];
    size_t rx_count = 0;
    network_stats_t stats;

    int res = network_simulator_transmit(tx, 50, &cfg, rx, 50, &rx_count, &stats, NULL, 0, NULL);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(stats.packets_reordered > 0);

    int order_inversion_found = 0;
    for (size_t i = 0; i + 1 < rx_count; i++) {
        if (rx[i].packet_id > rx[i + 1].packet_id) {
            order_inversion_found = 1;
            break;
        }
    }
    TEST_ASSERT(order_inversion_found == 1);
    printf("  [PASS] test_net_packet_reordering (Reordering created detectable sequence order inversion)\n");
}

/* Test 8: Combined impairments (Loss + Corruption + Reordering) */
static void test_net_combined_impairments(void) {
    simulated_packet_t tx[100];
    for (size_t i = 0; i < 100; i++) {
        packet_init(&tx[i], (uint32_t)i);
        tx[i].signaling_method = SIGNALING_METHOD_SEQUENCE;
        tx[i].sequence_number = 10000 + (uint32_t)(i * 100);
        tx[i].payload_length = 0;
    }

    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.loss_percent = 10;
    cfg.corruption_percent = 10;
    cfg.reorder_percent = 15;
    cfg.enable_loss = 1;
    cfg.enable_corruption = 1;
    cfg.enable_reordering = 1;
    cfg.random_seed = 777U;

    simulated_packet_t rx[100];
    size_t rx_count = 0;
    network_stats_t stats;

    int res = network_simulator_transmit(tx, 100, &cfg, rx, 100, &rx_count, &stats, NULL, 0, NULL);
    TEST_ASSERT(res == 0);
    TEST_ASSERT(stats.packets_lost > 0);
    TEST_ASSERT(stats.packets_corrupted > 0);
    TEST_ASSERT(stats.packets_reordered > 0);
    TEST_ASSERT(rx_count < 100);

    for (size_t i = 0; i < rx_count; i++) {
        TEST_ASSERT(rx[i].payload_length == 0);
    }
    printf("  [PASS] test_net_combined_impairments (Loss=%u, Corrupt=%u, Reorder=%u verified)\n",
           (unsigned int)stats.packets_lost, (unsigned int)stats.packets_corrupted, (unsigned int)stats.packets_reordered);
}

/* Test 9: Deterministic random seed */
static void test_net_deterministic_random_seed(void) {
    simulated_packet_t tx[40];
    for (size_t i = 0; i < 40; i++) {
        packet_init(&tx[i], (uint32_t)i);
        tx[i].signaling_method = SIGNALING_METHOD_SEQUENCE;
        tx[i].sequence_number = 10000 + (uint32_t)(i * 100);
        tx[i].payload_length = 0;
    }

    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.loss_percent = 15;
    cfg.corruption_percent = 15;
    cfg.reorder_percent = 20;
    cfg.enable_loss = 1;
    cfg.enable_corruption = 1;
    cfg.enable_reordering = 1;
    cfg.random_seed = 88888U;

    simulated_packet_t rx1[40], rx2[40];
    size_t rx1_count = 0, rx2_count = 0;
    network_stats_t stats1, stats2;

    network_simulator_transmit(tx, 40, &cfg, rx1, 40, &rx1_count, &stats1, NULL, 0, NULL);
    network_simulator_transmit(tx, 40, &cfg, rx2, 40, &rx2_count, &stats2, NULL, 0, NULL);

    TEST_ASSERT(rx1_count == rx2_count);
    TEST_ASSERT(stats1.packets_lost == stats2.packets_lost);
    TEST_ASSERT(stats1.packets_corrupted == stats2.packets_corrupted);
    TEST_ASSERT(stats1.packets_reordered == stats2.packets_reordered);

    for (size_t i = 0; i < rx1_count; i++) {
        TEST_ASSERT(rx1[i].packet_id == rx2[i].packet_id);
        TEST_ASSERT(rx1[i].sequence_number == rx2[i].sequence_number);
        TEST_ASSERT(rx1[i].window_size == rx2[i].window_size);
    }
    printf("  [PASS] test_net_deterministic_random_seed (Two runs with seed 88888 matched 100%%)\n");
}

/* Test 10: Payload remains 0 bytes invariant */
static void test_net_payload_remains_zero_bytes(void) {
    simulated_packet_t tx[50];
    for (size_t i = 0; i < 50; i++) {
        packet_init(&tx[i], (uint32_t)i);
        tx[i].payload_length = 0;
    }

    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.loss_percent = 30;
    cfg.corruption_percent = 30;
    cfg.reorder_percent = 30;
    cfg.enable_loss = 1;
    cfg.enable_corruption = 1;
    cfg.enable_reordering = 1;
    cfg.random_seed = 10101U;

    simulated_packet_t rx[50];
    size_t rx_count = 0;
    network_stats_t stats;

    network_simulator_transmit(tx, 50, &cfg, rx, 50, &rx_count, &stats, NULL, 0, NULL);

    for (size_t i = 0; i < rx_count; i++) {
        TEST_ASSERT(rx[i].payload_length == 0);
    }
    printf("  [PASS] test_net_payload_remains_zero_bytes (Strict 0 B payload invariant verified)\n");
}

/* Test 11: Sequence method still works normally with network simulator */
static void test_net_sequence_method_still_works_normally(void) {
    const char *msg = "HELLO NETWORK";
    uint8_t bits[256];
    size_t num_bits = 0;
    TEST_ASSERT(encoder_text_to_bits(msg, bits, 256, &num_bits) == 0);

    simulated_packet_t tx[256];
    size_t tx_count = 0;
    TEST_ASSERT(sequence_method_modulate(bits, num_bits, NULL, tx, 256, &tx_count) == SEQ_SUCCESS);

    network_config_t cfg;
    network_config_init_default(&cfg);

    simulated_packet_t rx[256];
    size_t rx_count = 0;
    network_stats_t stats;
    TEST_ASSERT(network_simulator_transmit(tx, tx_count, &cfg, rx, 256, &rx_count, &stats, NULL, 0, NULL) == 0);
    TEST_ASSERT(rx_count == tx_count);

    uint8_t rec_bits[256];
    size_t rec_count = 0;
    size_t invalid_idx = 0;
    TEST_ASSERT(sequence_method_demodulate(rx, rx_count, NULL, rec_bits, 256, &rec_count, &invalid_idx) == SEQ_SUCCESS);
    TEST_ASSERT(rec_count == num_bits);

    char decoded[64];
    size_t chars = 0;
    TEST_ASSERT(decoder_bits_to_text(rec_bits, rec_count, decoded, 64, &chars) == 0);
    TEST_ASSERT(strcmp(msg, decoded) == 0);
    TEST_ASSERT(verification_check_match(msg, decoded) == 1);
    printf("  [PASS] test_net_sequence_method_still_works_normally (Normal Sequence intact through net sim)\n");
}

/* Test 12: Window method still works normally with network simulator */
static void test_net_window_method_still_works_normally(void) {
    const char *msg = "HELLO NETWORK";
    uint8_t bits[256];
    size_t num_bits = 0;
    TEST_ASSERT(encoder_text_to_bits(msg, bits, 256, &num_bits) == 0);

    simulated_packet_t tx[256];
    size_t tx_count = 0;
    TEST_ASSERT(window_method_modulate(bits, num_bits, NULL, tx, 256, &tx_count) == WIN_SUCCESS);

    network_config_t cfg;
    network_config_init_default(&cfg);

    simulated_packet_t rx[256];
    size_t rx_count = 0;
    network_stats_t stats;
    TEST_ASSERT(network_simulator_transmit(tx, tx_count, &cfg, rx, 256, &rx_count, &stats, NULL, 0, NULL) == 0);
    TEST_ASSERT(rx_count == tx_count);

    uint8_t rec_bits[256];
    size_t rec_count = 0;
    size_t invalid_idx = 0;
    TEST_ASSERT(window_method_demodulate(rx, rx_count, NULL, rec_bits, 256, &rec_count, &invalid_idx) == WIN_SUCCESS);
    TEST_ASSERT(rec_count == num_bits);

    char decoded[64];
    size_t chars = 0;
    TEST_ASSERT(decoder_bits_to_text(rec_bits, rec_count, decoded, 64, &chars) == 0);
    TEST_ASSERT(strcmp(msg, decoded) == 0);
    TEST_ASSERT(verification_check_match(msg, decoded) == 1);
    printf("  [PASS] test_net_window_method_still_works_normally (Normal Window intact through net sim)\n");
}

/* =========================================================================
 * 5. COMPARATIVE ANALYSIS TESTS (Step 5 Requirements)
 * ========================================================================= */

/* Test 1: Compare mode exists */
static void test_comp_mode_exists(void) {
    const char *msg = "COMPARE TEST";
    network_config_t cfg;
    network_config_init_default(&cfg);
    comparison_result_t res;

    int rc = comparison_execute(msg, &cfg, &res);
    TEST_ASSERT(rc == 0);
    TEST_ASSERT(res.execution_success == 1);
    printf("  [PASS] test_comp_mode_exists (Comparison engine executes cleanly)\n");
}

/* Test 2: Same message used for both methods */
static void test_comp_same_message(void) {
    const char *msg = "EQUAL INPUT MESSAGE";
    network_config_t cfg;
    network_config_init_default(&cfg);
    comparison_result_t res;

    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(strcmp(res.original_message, msg) == 0);
    TEST_ASSERT(strcmp(res.sequence.decoded_message, msg) == 0);
    TEST_ASSERT(strcmp(res.window.decoded_message, msg) == 0);
    printf("  [PASS] test_comp_same_message (Same message provided to both methods)\n");
}

/* Test 3: Same network configuration used */
static void test_comp_same_network_config(void) {
    const char *msg = "CONFIG TEST";
    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.loss_percent = 12;
    cfg.corruption_percent = 8;
    cfg.reorder_percent = 15;
    cfg.enable_loss = 1;
    cfg.enable_corruption = 1;
    cfg.enable_reordering = 1;

    comparison_result_t res;
    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.net_cfg.loss_percent == 12);
    TEST_ASSERT(res.net_cfg.corruption_percent == 8);
    TEST_ASSERT(res.net_cfg.reorder_percent == 15);
    printf("  [PASS] test_comp_same_network_config (Identical network params applied)\n");
}

/* Test 4: Same random seed used */
static void test_comp_same_random_seed(void) {
    const char *msg = "SEED TEST";
    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.random_seed = 98765;

    comparison_result_t res;
    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.net_cfg.random_seed == 98765);
    printf("  [PASS] test_comp_same_random_seed (Exact random seed shared across methods)\n");
}

/* Test 5: Sequence result is returned */
static void test_comp_sequence_result_returned(void) {
    const char *msg = "HELLO";
    network_config_t cfg;
    network_config_init_default(&cfg);
    comparison_result_t res;

    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.sequence.packets_generated == 40);
    TEST_ASSERT(res.sequence.packets_received == 40);
    TEST_ASSERT(strcmp(res.sequence.method_id, "sequence") == 0);
    TEST_ASSERT(strcmp(res.sequence.method_name, "Sequence Number") == 0);
    printf("  [PASS] test_comp_sequence_result_returned (Sequence metrics populated)\n");
}

/* Test 6: Window result is returned */
static void test_comp_window_result_returned(void) {
    const char *msg = "HELLO";
    network_config_t cfg;
    network_config_init_default(&cfg);
    comparison_result_t res;

    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.window.packets_generated == 40);
    TEST_ASSERT(res.window.packets_received == 40);
    TEST_ASSERT(strcmp(res.window.method_id, "window") == 0);
    TEST_ASSERT(strcmp(res.window.method_name, "Window Size") == 0);
    printf("  [PASS] test_comp_window_result_returned (Window metrics populated)\n");
}

/* Test 7: Payload remains 0 */
static void test_comp_payload_strictly_zero(void) {
    const char *msg = "ZERO PAYLOAD INVARIANT";
    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.loss_percent = 10;
    cfg.corruption_percent = 10;
    cfg.reorder_percent = 10;

    comparison_result_t res;
    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.payload_bytes == 0);
    TEST_ASSERT(res.sequence.payload_bytes == 0);
    TEST_ASSERT(res.window.payload_bytes == 0);
    printf("  [PASS] test_comp_payload_strictly_zero (Strict 0 B payload invariant verified)\n");
}

/* Test 8: Normal comparison succeeds */
static void test_comp_normal_comparison_succeeds(void) {
    const char *msg = "NORMAL TEST";
    network_config_t cfg;
    network_config_init_default(&cfg);
    comparison_result_t res;

    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.sequence.integrity_match == 1);
    TEST_ASSERT(res.window.integrity_match == 1);
    TEST_ASSERT(strcmp(res.sequence.integrity_status, "verified") == 0);
    TEST_ASSERT(strcmp(res.window.integrity_status, "verified") == 0);
    TEST_ASSERT(res.sequence.packets_lost == 0);
    TEST_ASSERT(res.window.packets_lost == 0);
    printf("  [PASS] test_comp_normal_comparison_succeeds (Both methods 100%% verified in baseline)\n");
}

/* Test 9: Loss comparison works */
static void test_comp_loss_comparison_works(void) {
    const char *msg = "LOSS COMPARISON EXPERIMENT";
    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.loss_percent = 25;
    cfg.enable_loss = 1;
    cfg.random_seed = 44444;

    comparison_result_t res;
    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.sequence.packets_lost > 0);
    TEST_ASSERT(res.window.packets_lost > 0);
    TEST_ASSERT(res.sequence.packets_lost == res.window.packets_lost);
    TEST_ASSERT(res.sequence.packets_received == res.window.packets_received);
    printf("  [PASS] test_comp_loss_comparison_works (Loss rate evaluated equivalently)\n");
}

/* Test 10: Corruption comparison works */
static void test_comp_corruption_comparison_works(void) {
    const char *msg = "CORRUPTION COMPARISON";
    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.corruption_percent = 20;
    cfg.enable_corruption = 1;
    cfg.random_seed = 55555;

    comparison_result_t res;
    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.sequence.packets_corrupted > 0);
    TEST_ASSERT(res.window.packets_corrupted > 0);
    TEST_ASSERT(res.sequence.packets_corrupted == res.window.packets_corrupted);
    TEST_ASSERT(res.sequence.errors_detected > 0);
    TEST_ASSERT(res.window.errors_detected > 0);
    printf("  [PASS] test_comp_corruption_comparison_works (Corruptions tracked and detected)\n");
}

/* Test 11: Reordering comparison works */
static void test_comp_reordering_comparison_works(void) {
    const char *msg = "REORDER COMPARISON";
    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.reorder_percent = 30;
    cfg.enable_reordering = 1;
    cfg.random_seed = 66666;

    comparison_result_t res;
    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.sequence.packets_reordered > 0);
    TEST_ASSERT(res.window.packets_reordered > 0);
    TEST_ASSERT(res.sequence.packets_reordered == res.window.packets_reordered);
    printf("  [PASS] test_comp_reordering_comparison_works (Reordering evaluated equivalently)\n");
}

/* Test 12: Combined comparison works */
static void test_comp_combined_comparison_works(void) {
    const char *msg = "COMBINED COMPARISON TEST";
    network_config_t cfg;
    network_config_init_default(&cfg);
    cfg.loss_percent = 10;
    cfg.corruption_percent = 10;
    cfg.reorder_percent = 15;
    cfg.enable_loss = 1;
    cfg.enable_corruption = 1;
    cfg.enable_reordering = 1;
    cfg.random_seed = 77777;

    comparison_result_t res;
    TEST_ASSERT(comparison_execute(msg, &cfg, &res) == 0);
    TEST_ASSERT(res.sequence.packets_generated == res.window.packets_generated);
    TEST_ASSERT(res.sequence.packets_lost == res.window.packets_lost);
    TEST_ASSERT(res.sequence.packets_corrupted == res.window.packets_corrupted);
    TEST_ASSERT(res.sequence.packets_reordered == res.window.packets_reordered);
    TEST_ASSERT(res.payload_bytes == 0);
    printf("  [PASS] test_comp_combined_comparison_works (Combined impairments evaluated fairly)\n");
}

int run_backend_unit_tests(void) {
    g_test_failures = 0;

    printf("==================================================\n");
    printf("RUNNING C BACKEND UNIT TESTS\n");
    printf("==================================================\n");

    printf("\n--- 1. Sequence Number Signaling Tests (Step 2) ---\n");
    test_seq_bit_0_mapping();
    test_seq_bit_1_mapping();
    test_seq_number_generation();
    test_seq_demodulation();
    test_seq_payload_zero_invariant();
    test_seq_hello_round_trip();
    test_seq_longer_message_round_trip();
    test_seq_empty_input_handling();
    test_seq_invalid_delta_handling();

    printf("\n--- 2. Window Size Signaling Tests (Step 3) ---\n");
    test_win_bit_0_mapping();
    test_win_bit_1_mapping();
    test_win_modulation();
    test_win_demodulation();
    test_win_payload_always_zero();
    test_win_hello_round_trip();
    test_win_hello_network_round_trip();
    test_win_longer_message_round_trip();
    test_win_invalid_window_detection();

    printf("\n--- 3. Core Architectural & Component Tests ---\n");
    test_packet_structure();
    test_encoder_decoder();
    test_timing_signaling_placeholder();
    test_error_control();

    printf("\n--- 4. Network Impairment & Simulation Tests (Step 4) ---\n");
    test_net_normal_transmission();
    test_net_zero_packet_loss();
    test_net_packet_loss();
    test_net_packet_corruption();
    test_net_sequence_corruption_detection();
    test_net_window_corruption_detection();
    test_net_packet_reordering();
    test_net_combined_impairments();
    test_net_deterministic_random_seed();
    test_net_payload_remains_zero_bytes();
    test_net_sequence_method_still_works_normally();
    test_net_window_method_still_works_normally();

    printf("\n--- 5. Comparative Analysis Tests (Step 5) ---\n");
    test_comp_mode_exists();
    test_comp_same_message();
    test_comp_same_network_config();
    test_comp_same_random_seed();
    test_comp_sequence_result_returned();
    test_comp_window_result_returned();
    test_comp_payload_strictly_zero();
    test_comp_normal_comparison_succeeds();
    test_comp_loss_comparison_works();
    test_comp_corruption_comparison_works();
    test_comp_reordering_comparison_works();
    test_comp_combined_comparison_works();

    printf("\n==================================================\n");
    if (g_test_failures == 0) {
        printf("ALL C BACKEND TESTS PASSED (46/46)\n");
        printf("==================================================\n");
        return 0;
    } else {
        printf("C BACKEND TESTS FAILED: %d failure(s)\n", g_test_failures);
        printf("==================================================\n");
        return 1;
    }
}
