/**
 * @file verification.c
 * @brief Implementation of verification and BER calculation functions.
 */

#include "verification.h"
#include <string.h>

int verification_check_match(const char *original, const char *reconstructed) {
    if (!original || !reconstructed) return 0;
    return (strcmp(original, reconstructed) == 0) ? 1 : 0;
}

double verification_calculate_ber(const uint8_t *sent_bits, const uint8_t *recv_bits, size_t total_bits) {
    if (!sent_bits || !recv_bits || total_bits == 0) return 0.0;

    size_t errors = 0;
    for (size_t i = 0; i < total_bits; i++) {
        if (sent_bits[i] != recv_bits[i]) {
            errors++;
        }
    }

    return (double)errors / (double)total_bits;
}
