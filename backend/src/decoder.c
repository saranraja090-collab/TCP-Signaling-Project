/**
 * @file decoder.c
 * @brief Implementation of receiver bitstream decoding into plaintext.
 */

#include "decoder.h"

int decoder_bits_to_text(const uint8_t *bits, size_t num_bits,
                         char *text_out, size_t max_text,
                         size_t *chars_decoded) {
    if (!bits || !text_out || !chars_decoded) return -1;

    size_t byte_count = num_bits / 8;
    if (byte_count + 1 > max_text) return -1;

    for (size_t i = 0; i < byte_count; i++) {
        uint8_t byte = 0;
        for (int b = 7; b >= 0; b--) {
            byte |= (bits[i * 8 + (7 - b)] << b);
        }
        text_out[i] = (char)byte;
    }
    text_out[byte_count] = '\0';
    *chars_decoded = byte_count;
    return 0;
}
