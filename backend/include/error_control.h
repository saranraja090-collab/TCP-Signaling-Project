/**
 * @file error_control.h
 * @brief Error control architectures: CRC-16 and Hamming(7,4).
 *
 * Computer Networks Academic Project:
 * Error detection and correction mechanisms applied to signaling bitstreams.
 */

#ifndef TCP_SIGNALING_ERROR_CONTROL_H
#define TCP_SIGNALING_ERROR_CONTROL_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard CRC-CCITT polynomial (0x1021) */
#define CRC16_POLYNOMIAL 0x1021

/**
 * @brief Compute standard CRC-16 over a buffer of bytes.
 *
 * @param data Byte buffer.
 * @param len Length in bytes.
 * @return 16-bit CRC value.
 */
uint16_t error_control_crc16(const uint8_t *data, size_t len);

/**
 * @brief Verify data against an expected CRC-16 checksum.
 *
 * @param data Byte buffer.
 * @param len Length in bytes.
 * @param expected_crc Expected 16-bit checksum.
 * @return 1 if valid, 0 if corrupted.
 */
int error_control_verify_crc16(const uint8_t *data, size_t len, uint16_t expected_crc);

/**
 * @brief Encode a 4-bit nibble into a 7-bit Hamming(7,4) codeword.
 *
 * Codeword format: [p1, p2, d1, p3, d2, d3, d4]
 *
 * @param data_nibble 4 data bits (bits 0..3).
 * @return 7-bit codeword.
 */
uint8_t error_control_hamming_encode_nibble(uint8_t data_nibble);

/**
 * @brief Decode a 7-bit Hamming(7,4) codeword, detecting and correcting single-bit errors.
 *
 * @param codeword 7-bit codeword.
 * @param corrected_out Pointer set to 1 if a bit error was detected and repaired, 0 otherwise.
 * @param error_pos_out Pointer set to the 1-based error position (0 if clean).
 * @return Recovered 4-bit data nibble.
 */
uint8_t error_control_hamming_decode_nibble(uint8_t codeword, int *corrected_out, int *error_pos_out);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SIGNALING_ERROR_CONTROL_H */
