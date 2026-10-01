/**
 * @file decoder.h
 * @brief Receiver decoding and message reconstruction.
 */

#ifndef TCP_SIGNALING_DECODER_H
#define TCP_SIGNALING_DECODER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Decode an array of bits back into a C string.
 *
 * Consumes 8 bits per character.
 *
 * @param bits Array of 0 and 1 values.
 * @param num_bits Total number of bits (must be multiple of 8 for full chars).
 * @param text_out Output buffer for reconstructed string.
 * @param max_text Maximum length of text_out.
 * @param chars_decoded Output receiving count of characters reconstructed.
 * @return 0 on success, -1 on error.
 */
int decoder_bits_to_text(const uint8_t *bits, size_t num_bits,
                         char *text_out, size_t max_text,
                         size_t *chars_decoded);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SIGNALING_DECODER_H */
