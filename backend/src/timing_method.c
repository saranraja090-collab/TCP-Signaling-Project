/**
 * @file timing_method.c
 * @brief Implementation of timing-based modulation and demodulation.
 */

#include "timing_method.h"

void timing_method_init_config(timing_method_config_t *config) {
    if (!config) return;
    config->delay_zero_ms = 15.0;
    config->delay_one_ms  = 45.0;
}

int timing_method_modulate(const uint8_t *bits, size_t num_bits,
                           const timing_method_config_t *config,
                           simulated_packet_t *packets_out, size_t max_packets,
                           size_t *packets_created) {
    if (!bits || !packets_out || !packets_created) return -1;
    if (num_bits > max_packets) return -1;

    timing_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        timing_method_init_config(&cfg);
    }

    double current_time_ms = 0.0;

    for (size_t i = 0; i < num_bits; i++) {
        packet_init(&packets_out[i], (uint32_t)i);
        packets_out[i].signaling_method = SIGNALING_METHOD_TIMING;
        packets_out[i].signaled_symbol = bits[i];

        /* Delay relative to previous packet */
        double delay = (bits[i] == 1) ? cfg.delay_one_ms : cfg.delay_zero_ms;
        current_time_ms += delay;

        packets_out[i].inter_packet_gap_ms = delay;
        packets_out[i].timestamp_ms = current_time_ms;

        /* Keep simulated payload strictly 0 bytes */
        packets_out[i].payload_length = 0;
    }

    *packets_created = num_bits;
    return 0;
}

int timing_method_demodulate(const simulated_packet_t *packets, size_t num_packets,
                             const timing_method_config_t *config,
                             uint8_t *bits_out, size_t max_bits,
                             size_t *bits_recovered) {
    if (!packets || !bits_out || !bits_recovered) return -1;
    if (num_packets > max_bits) return -1;

    timing_method_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        timing_method_init_config(&cfg);
    }

    double threshold = (cfg.delay_zero_ms + cfg.delay_one_ms) / 2.0;

    for (size_t i = 0; i < num_packets; i++) {
        bits_out[i] = (packets[i].inter_packet_gap_ms > threshold) ? 1 : 0;
    }

    *bits_recovered = num_packets;
    return 0;
}
