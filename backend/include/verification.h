/**
 * @file verification.h
 * @brief Message integrity verification and Bit Error Rate (BER) analysis.
 */

#ifndef TCP_SIGNALING_VERIFICATION_H
#define TCP_SIGNALING_VERIFICATION_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compare original plaintext with reconstructed message.
 *
 * @param original Original source string.
 * @param reconstructed Reconstructed message string.
 * @return 1 if identical, 0 if mismatch or error.
 */
int verification_check_match(const char *original, const char *reconstructed);

/**
 * @brief Compute the Bit Error Rate (BER) between transmitted and received bit arrays.
 *
 * BER = (number of bit errors) / (total bits)
 *
 * @param sent_bits Transmitted bit array.
 * @param recv_bits Received bit array.
 * @param total_bits Total bit count.
 * @return BER as a float from 0.0 (error-free) to 1.0.
 */
double verification_calculate_ber(const uint8_t *sent_bits, const uint8_t *recv_bits, size_t total_bits);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SIGNALING_VERIFICATION_H */
