/**
 * @file encoder.h
 * @brief Text and binary bitstream encoding interface.
 *
 * Provides functions to translate application messages into binary bitstreams
 * (arrays of 0 and 1 bits) ready for transport-layer modulation.
 */

#ifndef TCP_SIGNALING_ENCODER_H
#define TCP_SIGNALING_ENCODER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert a C string to a bitstream of 0 and 1 values.
 *
 * Each byte is converted into 8 bits (MSB first).
 *
 * @param text Null-terminated input string.
 * @param bits_out Output buffer for bits (each uint8_t is 0 or 1).
 * @param max_bits Capacity of bits_out buffer.
 * @param out_bit_count Output variable receiving the total number of bits written.
 * @return 0 on success, -1 on buffer overflow or null parameter.
 */
int encoder_text_to_bits(const char *text, uint8_t *bits_out, size_t max_bits, size_t *out_bit_count);

/**
 * @brief Format a bitstream into a readable string of '0' and '1' characters.
 *
 * @param bits Array of 0 and 1 values.
 * @param num_bits Number of bits.
 * @param str_out Output buffer for ASCII characters ('0'/'1').
 * @param max_str Capacity of str_out buffer (must be at least num_bits + 1).
 * @return 0 on success, -1 on error.
 */
int encoder_bits_to_string(const uint8_t *bits, size_t num_bits, char *str_out, size_t max_str);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SIGNALING_ENCODER_H */
