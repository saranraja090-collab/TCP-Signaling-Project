/**
 * @file error_control.c
 * @brief Implementation of CRC-16 and Hamming(7,4) error control algorithms.
 */

#include "error_control.h"

uint16_t error_control_crc16(const uint8_t *data, size_t len) {
    if (!data) return 0;

    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i] << 8);
        for (int b = 0; b < 8; b++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC16_POLYNOMIAL;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

int error_control_verify_crc16(const uint8_t *data, size_t len, uint16_t expected_crc) {
    return (error_control_crc16(data, len) == expected_crc) ? 1 : 0;
}

uint8_t error_control_hamming_encode_nibble(uint8_t data_nibble) {
    /* Extract 4 data bits: d1, d2, d3, d4 */
    uint8_t d1 = (data_nibble >> 0) & 1;
    uint8_t d2 = (data_nibble >> 1) & 1;
    uint8_t d3 = (data_nibble >> 2) & 1;
    uint8_t d4 = (data_nibble >> 3) & 1;

    /* Compute parity bits according to standard Hamming(7,4) matrix */
    uint8_t p1 = d1 ^ d2 ^ d4;
    uint8_t p2 = d1 ^ d3 ^ d4;
    uint8_t p3 = d2 ^ d3 ^ d4;

    /* Bit positions: 1:p1, 2:p2, 3:d1, 4:p3, 5:d2, 6:d3, 7:d4 */
    uint8_t codeword = (p1 << 0) | (p2 << 1) | (d1 << 2) |
                       (p3 << 3) | (d2 << 4) | (d3 << 5) | (d4 << 6);
    return codeword & 0x7F;
}

uint8_t error_control_hamming_decode_nibble(uint8_t codeword, int *corrected_out, int *error_pos_out) {
    uint8_t b1 = (codeword >> 0) & 1;
    uint8_t b2 = (codeword >> 1) & 1;
    uint8_t b3 = (codeword >> 2) & 1;
    uint8_t b4 = (codeword >> 3) & 1;
    uint8_t b5 = (codeword >> 4) & 1;
    uint8_t b6 = (codeword >> 5) & 1;
    uint8_t b7 = (codeword >> 6) & 1;

    /* Calculate syndrome bits */
    uint8_t s1 = b1 ^ b3 ^ b5 ^ b7;
    uint8_t s2 = b2 ^ b3 ^ b6 ^ b7;
    uint8_t s3 = b4 ^ b5 ^ b6 ^ b7;

    uint8_t syndrome = (s3 << 2) | (s2 << 1) | (s1 << 0);

    if (error_pos_out) *error_pos_out = syndrome;
    if (corrected_out) *corrected_out = (syndrome != 0) ? 1 : 0;

    /* Correct bit if syndrome is non-zero (1-indexed position) */
    if (syndrome >= 1 && syndrome <= 7) {
        codeword ^= (1 << (syndrome - 1));
    }

    /* Extract repaired data bits */
    uint8_t d1 = (codeword >> 2) & 1;
    uint8_t d2 = (codeword >> 4) & 1;
    uint8_t d3 = (codeword >> 5) & 1;
    uint8_t d4 = (codeword >> 6) & 1;

    return (d1 << 0) | (d2 << 1) | (d3 << 2) | (d4 << 3);
}
