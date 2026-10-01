/**
 * @file sequence_method.c
 * @brief Implementation of sequence-number-based modulation, demodulation, and delta validation.
 */

#include "sequence_method.h"
#include <stdlib.h>

void sequence_method_init_config(sequence_method_config_t *config) {
    if (!config) return;
    config->base_sequence_number = SEQ_DEFAULT_BASE;
    config->step_zero            = SEQ_INCREMENT_0;
    config->step_one             = SEQ_INCREMENT_1;
    config->delta_tolerance      = SEQ_DELTA_TOLERANCE;
}

uint32_t sequence_get_increment_for_bit(uint8_t bit, const sequence_method_config_t *config) {
    sequence_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        sequence_method_init_config(&cfg);
    }

    return (bit == 1) ? cfg.step_one : cfg.step_zero;
}

int sequence_get_bit_from_delta(uint32_t delta, const sequence_method_config_t *config, uint8_t *bit_out) {
    if (!bit_out) return SEQ_ERR_NULL_PARAM;

    sequence_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        sequence_method_init_config(&cfg);
    }

    long diff_zero = labs((long)delta - (long)cfg.step_zero);
    long diff_one  = labs((long)delta - (long)cfg.step_one);

    if (diff_zero <= (long)cfg.delta_tolerance) {
        *bit_out = 0;
        return SEQ_SUCCESS;
    }

    if (diff_one <= (long)cfg.delta_tolerance) {
        *bit_out = 1;
        return SEQ_SUCCESS;
    }

    /* Unrecognized sequence increment outside allowable tolerance */
    return SEQ_ERR_INVALID_DELTA;
}

int sequence_method_modulate(const uint8_t *bits, size_t num_bits,
                             const sequence_method_config_t *config,
                             simulated_packet_t *packets_out, size_t max_packets,
                             size_t *packets_created) {
    if (!packets_out || !packets_created) return SEQ_ERR_NULL_PARAM;
    if (num_bits > 0 && !bits) return SEQ_ERR_NULL_PARAM;
    if (num_bits > max_packets) return SEQ_ERR_BUFFER_OVERFLOW;

    sequence_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        sequence_method_init_config(&cfg);
    }

    if (num_bits == 0) {
        *packets_created = 0;
        return SEQ_SUCCESS;
    }

    uint32_t current_seq = cfg.base_sequence_number;

    for (size_t i = 0; i < num_bits; i++) {
        packet_init(&packets_out[i], (uint32_t)i);
        packets_out[i].signaling_method = SIGNALING_METHOD_SEQUENCE;
        packets_out[i].signaled_symbol  = bits[i] ? 1 : 0;

        /* Increment sequence number according to bit value */
        uint32_t delta = sequence_get_increment_for_bit(bits[i], &cfg);
        current_seq += delta;
        packets_out[i].sequence_number = current_seq;

        /* Critical Invariant: simulated application payload is strictly 0 bytes */
        packets_out[i].payload_length = 0;

        /* Monotonic simulated departure telemetry */
        packets_out[i].timestamp_ms = (double)i * 10.0;
        packets_out[i].inter_packet_gap_ms = 10.0;
        packets_out[i].flags = TCP_FLAG_ACK;
        packets_out[i].window_size = 65535;
        packets_out[i].acknowledgment_number = 1;
    }

    *packets_created = num_bits;
    return SEQ_SUCCESS;
}

int sequence_method_demodulate(const simulated_packet_t *packets, size_t num_packets,
                               const sequence_method_config_t *config,
                               uint8_t *bits_out, size_t max_bits,
                               size_t *bits_recovered,
                               size_t *invalid_packet_idx) {
    if (!bits_out || !bits_recovered) return SEQ_ERR_NULL_PARAM;
    if (num_packets > 0 && !packets) return SEQ_ERR_NULL_PARAM;
    if (num_packets > max_bits) return SEQ_ERR_BUFFER_OVERFLOW;

    if (invalid_packet_idx) *invalid_packet_idx = 0;

    sequence_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        sequence_method_init_config(&cfg);
    }

    if (num_packets == 0) {
        *bits_recovered = 0;
        return SEQ_SUCCESS;
    }

    uint32_t prev_seq = cfg.base_sequence_number;

    for (size_t i = 0; i < num_packets; i++) {
        uint32_t current_seq = packets[i].sequence_number;
        uint32_t delta = current_seq - prev_seq;

        uint8_t bit = 0;
        int status = sequence_get_bit_from_delta(delta, &cfg, &bit);
        if (status != SEQ_SUCCESS) {
            if (invalid_packet_idx) *invalid_packet_idx = i;
            return SEQ_ERR_INVALID_DELTA;
        }

        bits_out[i] = bit;
        prev_seq = current_seq;
    }

    *bits_recovered = num_packets;
    return SEQ_SUCCESS;
}
