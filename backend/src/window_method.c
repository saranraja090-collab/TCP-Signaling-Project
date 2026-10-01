/**
 * @file window_method.c
 * @brief Implementation of window-size-based modulation, demodulation, and value validation.
 */

#include "window_method.h"
#include <stdlib.h>

void window_method_init_config(window_method_config_t *config) {
    if (!config) return;
    config->window_zero        = WINDOW_VALUE_0;
    config->window_one         = WINDOW_VALUE_1;
    config->window_tolerance   = WINDOW_VALUE_TOLERANCE;
    config->baseline_sequence  = 10000U;
}

uint16_t window_get_size_for_bit(uint8_t bit, const window_method_config_t *config) {
    window_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        window_method_init_config(&cfg);
    }

    return (bit == 1) ? cfg.window_one : cfg.window_zero;
}

int window_get_bit_from_size(uint16_t window_size, const window_method_config_t *config, uint8_t *bit_out) {
    if (!bit_out) return WIN_ERR_NULL_PARAM;

    window_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        window_method_init_config(&cfg);
    }

    long diff_zero = labs((long)window_size - (long)cfg.window_zero);
    long diff_one  = labs((long)window_size - (long)cfg.window_one);

    if (diff_zero <= (long)cfg.window_tolerance) {
        *bit_out = 0;
        return WIN_SUCCESS;
    }

    if (diff_one <= (long)cfg.window_tolerance) {
        *bit_out = 1;
        return WIN_SUCCESS;
    }

    /* Unexpected window size: do NOT silently interpret an unknown value */
    return WIN_ERR_INVALID_WINDOW;
}

int window_method_modulate(const uint8_t *bits, size_t num_bits,
                           const window_method_config_t *config,
                           simulated_packet_t *packets_out, size_t max_packets,
                           size_t *packets_created) {
    if (!packets_out || !packets_created) return WIN_ERR_NULL_PARAM;
    if (num_bits > 0 && !bits) return WIN_ERR_NULL_PARAM;
    if (num_bits > max_packets) return WIN_ERR_BUFFER_OVERFLOW;

    window_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        window_method_init_config(&cfg);
    }

    if (num_bits == 0) {
        *packets_created = 0;
        return WIN_SUCCESS;
    }

    for (size_t i = 0; i < num_bits; i++) {
        packet_init(&packets_out[i], (uint32_t)i);
        packets_out[i].signaling_method = SIGNALING_METHOD_WINDOW;
        packets_out[i].signaled_symbol  = bits[i] ? 1 : 0;

        /* Modulate 16-bit window size: 0 -> 30000, 1 -> 60000 */
        packets_out[i].window_size = window_get_size_for_bit(bits[i], &cfg);

        /* Standard TCP control header simulation */
        packets_out[i].sequence_number = cfg.baseline_sequence + (uint32_t)i;
        packets_out[i].acknowledgment_number = 1;
        packets_out[i].flags = TCP_FLAG_ACK;

        /* Critical Invariant: simulated application payload is strictly 0 bytes */
        packets_out[i].payload_length = 0;

        /* Monotonic simulated departure telemetry */
        packets_out[i].timestamp_ms = (double)i * 10.0;
        packets_out[i].inter_packet_gap_ms = 10.0;
    }

    *packets_created = num_bits;
    return WIN_SUCCESS;
}

int window_method_demodulate(const simulated_packet_t *packets, size_t num_packets,
                             const window_method_config_t *config,
                             uint8_t *bits_out, size_t max_bits,
                             size_t *bits_recovered,
                             size_t *invalid_packet_idx) {
    if (!bits_out || !bits_recovered) return WIN_ERR_NULL_PARAM;
    if (num_packets > 0 && !packets) return WIN_ERR_NULL_PARAM;
    if (num_packets > max_bits) return WIN_ERR_BUFFER_OVERFLOW;

    if (invalid_packet_idx) *invalid_packet_idx = 0;

    window_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        window_method_init_config(&cfg);
    }

    if (num_packets == 0) {
        *bits_recovered = 0;
        return WIN_SUCCESS;
    }

    for (size_t i = 0; i < num_packets; i++) {
        uint8_t bit = 0;
        int status = window_get_bit_from_size(packets[i].window_size, &cfg, &bit);
        if (status != WIN_SUCCESS) {
            if (invalid_packet_idx) *invalid_packet_idx = i;
            return WIN_ERR_INVALID_WINDOW;
        }

        bits_out[i] = bit;
    }

    *bits_recovered = num_packets;
    return WIN_SUCCESS;
}
