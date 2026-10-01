/**
 * @file encoder.c
 * @brief Implementation of text and bitstream encoding routines.
 */

#include "encoder.h"
#include <string.h>

int encoder_text_to_bits(const char *text, uint8_t *bits_out, size_t max_bits, size_t *out_bit_count) {
    if (!text || !bits_out || !out_bit_count) return -1;

    size_t text_len = strlen(text);
    size_t required_bits = text_len * 8;

    if (required_bits > max_bits) return -1;

    size_t bit_idx = 0;
    for (size_t i = 0; i < text_len; i++) {
        uint8_t byte = (uint8_t)text[i];
        for (int b = 7; b >= 0; b--) {
            bits_out[bit_idx++] = (byte >> b) & 1;
        }
    }

    *out_bit_count = bit_idx;
    return 0;
}

int encoder_bits_to_string(const uint8_t *bits, size_t num_bits, char *str_out, size_t max_str) {
    if (!bits || !str_out || max_str <= num_bits) return -1;

    for (size_t i = 0; i < num_bits; i++) {
        str_out[i] = bits[i] ? '1' : '0';
    }
    str_out[num_bits] = '\0';
    return 0;
}
