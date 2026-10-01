/**
 * @file comparison.c
 * @brief Implementation of Comparative Analysis Engine for Sequence and Window Signaling.
 *
 * Step 5 — Comparative Analysis of Sequence and Window Signaling:
 * Compares Sequence Number Signaling vs Window Size Signaling under equivalent conditions.
 *
 * STRICT REQUIREMENTS:
 * - Same message for both methods
 * - Same network configuration (loss, corruption, reordering)
 * - Same random seed (re-seeded before each run for exact reproducibility)
 * - Strict 0-byte payload invariant on all packets
 * - NO subjective scoring, winners, or rankings
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comparison.h"
#include "encoder.h"
#include "sequence_method.h"
#include "window_method.h"
#include "decoder.h"
#include "verification.h"

#define COMP_MAX_BITS_BUFFER    8192
#define COMP_MAX_PACKET_BUFFER  8192
#define COMP_MAX_FLOW_RECORDS   256

static void json_print_escaped_string(const char *str) {
    if (!str) {
        printf("\"\"");
        return;
    }
    putchar('"');
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  printf("\\\""); break;
            case '\\': printf("\\\\"); break;
            case '\b': printf("\\b");  break;
            case '\f': printf("\\f");  break;
            case '\n': printf("\\n");  break;
            case '\r': printf("\\r");  break;
            case '\t': printf("\\t");  break;
            default:
                if ((unsigned char)*p < 0x20 || (unsigned char)*p >= 0x7F) {
                    printf("\\u%04x", (unsigned char)*p);
                } else {
                    putchar(*p);
                }
        }
    }
    putchar('"');
}

static int simulate_method(signaling_method_t method,
                           const char *input_msg,
                           const uint8_t *bits,
                           size_t num_bits,
                           const network_config_t *net_cfg,
                           comparison_method_metrics_t *metrics) {
    if (!input_msg || !bits || !net_cfg || !metrics) return -1;

    memset(metrics, 0, sizeof(comparison_method_metrics_t));
    metrics->method = method;
    metrics->payload_bytes = 0;

    if (method == SIGNALING_METHOD_SEQUENCE) {
        strncpy(metrics->method_id, "sequence", sizeof(metrics->method_id) - 1);
        strncpy(metrics->method_name, "Sequence Number", sizeof(metrics->method_name) - 1);
    } else {
        strncpy(metrics->method_id, "window", sizeof(metrics->method_id) - 1);
        strncpy(metrics->method_name, "Window Size", sizeof(metrics->method_name) - 1);
    }

    /* 1. Packet Generation / Modulation */
    static simulated_packet_t tx_packets[COMP_MAX_PACKET_BUFFER];
    size_t tx_count = 0;

    if (method == SIGNALING_METHOD_SEQUENCE) {
        int mod_res = sequence_method_modulate(bits, num_bits, NULL, tx_packets, COMP_MAX_PACKET_BUFFER, &tx_count);
        if (mod_res != SEQ_SUCCESS) return -1;
    } else if (method == SIGNALING_METHOD_WINDOW) {
        int mod_res = window_method_modulate(bits, num_bits, NULL, tx_packets, COMP_MAX_PACKET_BUFFER, &tx_count);
        if (mod_res != WIN_SUCCESS) return -1;
    } else {
        return -1;
    }

    /* Enforce 0-byte payload invariant */
    for (size_t i = 0; i < tx_count; i++) {
        tx_packets[i].payload_length = 0;
    }

    /* 2. In-Memory Simulated Network Transmission */
    static simulated_packet_t rx_packets[COMP_MAX_PACKET_BUFFER];
    size_t rx_count = 0;
    network_stats_t net_stats;
    static packet_flow_record_t flow_records[COMP_MAX_FLOW_RECORDS];
    size_t flow_count = 0;

    /* Create local copy of config so PRNG starts from exact configured seed */
    network_config_t local_cfg = *net_cfg;
    int sim_res = network_simulator_transmit(tx_packets, tx_count, &local_cfg,
                                            rx_packets, COMP_MAX_PACKET_BUFFER, &rx_count,
                                            &net_stats, flow_records, COMP_MAX_FLOW_RECORDS, &flow_count);
    if (sim_res != 0) return -1;

    /* Enforce 0-byte payload invariant on arrival */
    for (size_t i = 0; i < rx_count; i++) {
        rx_packets[i].payload_length = 0;
    }

    metrics->packets_generated = net_stats.packets_generated;
    metrics->packets_received  = net_stats.packets_received;
    metrics->packets_lost      = net_stats.packets_lost;
    metrics->packets_corrupted = net_stats.packets_corrupted;
    metrics->packets_reordered = net_stats.packets_reordered;

    /* 3. Receiver Anomaly & Disruption Detection */
    int order_error = 0;
    int loss_disruption = 0;
    int corruption_detected = 0;

    if (rx_count > 1) {
        for (size_t i = 0; i + 1 < rx_count; i++) {
            if (method == SIGNALING_METHOD_SEQUENCE) {
                if (rx_packets[i + 1].sequence_number <= rx_packets[i].sequence_number ||
                    rx_packets[i + 1].packet_id != rx_packets[i].packet_id + 1) {
                    order_error = 1;
                    break;
                }
            } else if (method == SIGNALING_METHOD_WINDOW) {
                if (rx_packets[i + 1].packet_id != rx_packets[i].packet_id + 1) {
                    order_error = 1;
                    break;
                }
            }
        }
    }

    if (net_stats.packets_lost > 0 || rx_count < tx_count) {
        loss_disruption = 1;
    }

    /* 4. Demodulation */
    uint8_t recovered_bits[COMP_MAX_BITS_BUFFER];
    size_t bits_recovered = 0;
    size_t invalid_idx = 0;

    int dem_status = 0;
    if (method == SIGNALING_METHOD_SEQUENCE) {
        dem_status = sequence_method_demodulate(rx_packets, rx_count, NULL,
                                                recovered_bits, COMP_MAX_BITS_BUFFER,
                                                &bits_recovered, &invalid_idx);
        if (dem_status != SEQ_SUCCESS) {
            corruption_detected = 1;
        }
    } else if (method == SIGNALING_METHOD_WINDOW) {
        dem_status = window_method_demodulate(rx_packets, rx_count, NULL,
                                              recovered_bits, COMP_MAX_BITS_BUFFER,
                                              &bits_recovered, &invalid_idx);
        if (dem_status != WIN_SUCCESS) {
            corruption_detected = 1;
        }
    }

    /* Record detected protocol errors */
    metrics->num_detected_errors = 0;
    if (corruption_detected) {
        const char *err_tag = (method == SIGNALING_METHOD_SEQUENCE) ? "INVALID_SEQUENCE_DELTA" : "INVALID_WINDOW_VALUE";
        strncpy(metrics->detected_errors[metrics->num_detected_errors++], err_tag, 63);
        metrics->errors_detected++;
    }
    if (order_error) {
        strncpy(metrics->detected_errors[metrics->num_detected_errors++], "PACKET_ORDER_ERROR", 63);
        metrics->errors_detected++;
    }
    if (loss_disruption) {
        strncpy(metrics->detected_errors[metrics->num_detected_errors++], "PACKET_LOSS_DISRUPTION", 63);
        metrics->errors_detected += net_stats.packets_lost;
    }

    /* 5. UTF-8 Text Reconstruction */
    size_t chars_decoded = 0;
    if (bits_recovered > 0) {
        decoder_bits_to_text(recovered_bits, bits_recovered,
                             metrics->decoded_message, COMP_MAX_TEXT_LEN,
                             &chars_decoded);
    } else {
        metrics->decoded_message[0] = '\0';
    }

    /* 6. Integrity Verification & BER Analysis */
    metrics->integrity_match = 0;
    if (metrics->errors_detected == 0 && dem_status == 0 && bits_recovered == num_bits) {
        metrics->integrity_match = verification_check_match(input_msg, metrics->decoded_message);
    }

    if (metrics->integrity_match) {
        metrics->bit_error_rate = 0.0;
        strncpy(metrics->integrity_status, "verified", sizeof(metrics->integrity_status) - 1);
    } else if (loss_disruption && rx_count > 0) {
        metrics->bit_error_rate = (bits_recovered == 0) ? 1.0 : (double)(num_bits - bits_recovered) / (double)num_bits;
        strncpy(metrics->integrity_status, "incomplete", sizeof(metrics->integrity_status) - 1);
    } else {
        metrics->bit_error_rate = (bits_recovered == num_bits) ?
            verification_calculate_ber(bits, recovered_bits, num_bits) : 1.0;
        strncpy(metrics->integrity_status, "failed", sizeof(metrics->integrity_status) - 1);
    }

    return 0;
}

int comparison_execute(const char *input_msg, const network_config_t *net_cfg, comparison_result_t *out_result) {
    if (!input_msg || !net_cfg || !out_result) return -1;

    memset(out_result, 0, sizeof(comparison_result_t));
    strncpy(out_result->original_message, input_msg, COMP_MAX_TEXT_LEN - 1);
    out_result->net_cfg = *net_cfg;
    out_result->payload_bytes = 0;

    /* Binary encoding of original message */
    uint8_t bits[COMP_MAX_BITS_BUFFER];
    size_t num_bits = 0;
    if (encoder_text_to_bits(input_msg, bits, COMP_MAX_BITS_BUFFER, &num_bits) != 0) {
        return -1;
    }

    /* 1. Simulate Sequence Number Signaling */
    if (simulate_method(SIGNALING_METHOD_SEQUENCE, input_msg, bits, num_bits, net_cfg, &out_result->sequence) != 0) {
        return -1;
    }

    /* 2. Simulate Window Size Signaling under identical conditions & seed */
    if (simulate_method(SIGNALING_METHOD_WINDOW, input_msg, bits, num_bits, net_cfg, &out_result->window) != 0) {
        return -1;
    }

    out_result->execution_success = 1;
    return 0;
}

void comparison_print_cli(const comparison_result_t *res) {
    if (!res) return;

    printf("========================================\n");
    printf("SIGNALING METHOD COMPARISON\n");
    printf("========================================\n\n");

    printf("Original Message:\n%s\n\n", res->original_message);

    printf("Network Conditions:\n");
    printf("Loss       : %u%%\n", res->net_cfg.loss_percent);
    printf("Corruption : %u%%\n", res->net_cfg.corruption_percent);
    printf("Reordering : %u%%\n", res->net_cfg.reorder_percent);
    printf("Seed       : %u\n\n", res->net_cfg.random_seed);

    /* Method 1: Sequence Number */
    printf("----------------------------------------\n");
    printf("SEQUENCE NUMBER\n");
    printf("----------------------------------------\n\n");

    printf("Packets Generated : %u\n", (unsigned int)res->sequence.packets_generated);
    printf("Packets Received  : %u\n", (unsigned int)res->sequence.packets_received);
    printf("Packets Lost      : %u\n", (unsigned int)res->sequence.packets_lost);
    printf("Packets Corrupted : %u\n", (unsigned int)res->sequence.packets_corrupted);
    printf("Packets Reordered : %u\n", (unsigned int)res->sequence.packets_reordered);
    printf("Payload           : 0 bytes\n\n");

    printf("Decoded Message:\n%s\n\n", res->sequence.decoded_message);

    printf("Integrity:\n%s\n\n", (res->sequence.integrity_match) ? "VERIFIED" :
           (strcmp(res->sequence.integrity_status, "incomplete") == 0) ? "INCOMPLETE" : "FAILED");

    /* Method 2: Window Size */
    printf("----------------------------------------\n");
    printf("WINDOW SIZE\n");
    printf("----------------------------------------\n\n");

    printf("Packets Generated : %u\n", (unsigned int)res->window.packets_generated);
    printf("Packets Received  : %u\n", (unsigned int)res->window.packets_received);
    printf("Packets Lost      : %u\n", (unsigned int)res->window.packets_lost);
    printf("Packets Corrupted : %u\n", (unsigned int)res->window.packets_corrupted);
    printf("Packets Reordered : %u\n", (unsigned int)res->window.packets_reordered);
    printf("Payload           : 0 bytes\n\n");

    printf("Decoded Message:\n%s\n\n", res->window.decoded_message);

    printf("Integrity:\n%s\n\n", (res->window.integrity_match) ? "VERIFIED" :
           (strcmp(res->window.integrity_status, "incomplete") == 0) ? "INCOMPLETE" : "FAILED");

    printf("========================================\n");
}

void comparison_print_json(const comparison_result_t *res) {
    if (!res) return;

    printf("{\n");
    printf("  \"status\": \"%s\",\n", res->execution_success ? "success" : "failed");
    printf("  \"mode\": \"comparison\",\n");

    printf("  \"experiment\": {\n");
    printf("    \"message\": ");
    json_print_escaped_string(res->original_message);
    printf(",\n");
    printf("    \"loss_percent\": %u,\n", res->net_cfg.loss_percent);
    printf("    \"corruption_percent\": %u,\n", res->net_cfg.corruption_percent);
    printf("    \"reorder_percent\": %u,\n", res->net_cfg.reorder_percent);
    printf("    \"random_seed\": %u\n", res->net_cfg.random_seed);
    printf("  },\n");

    printf("  \"payload_bytes\": 0,\n");

    /* Sequence Section */
    printf("  \"sequence\": {\n");
    printf("    \"method\": \"sequence\",\n");
    printf("    \"packets_generated\": %u,\n", (unsigned int)res->sequence.packets_generated);
    printf("    \"packets_received\": %u,\n", (unsigned int)res->sequence.packets_received);
    printf("    \"packets_lost\": %u,\n", (unsigned int)res->sequence.packets_lost);
    printf("    \"packets_corrupted\": %u,\n", (unsigned int)res->sequence.packets_corrupted);
    printf("    \"packets_reordered\": %u,\n", (unsigned int)res->sequence.packets_reordered);
    printf("    \"errors_detected\": %u,\n", (unsigned int)res->sequence.errors_detected);
    printf("    \"decoded_message\": ");
    json_print_escaped_string(res->sequence.decoded_message);
    printf(",\n");
    printf("    \"integrity_match\": %s,\n", res->sequence.integrity_match ? "true" : "false");
    printf("    \"integrity\": \"%s\",\n", res->sequence.integrity_status);
    printf("    \"detected_errors\": [");
    for (size_t i = 0; i < res->sequence.num_detected_errors; i++) {
        printf("\"%s\"%s", res->sequence.detected_errors[i], (i + 1 < res->sequence.num_detected_errors) ? ", " : "");
    }
    printf("]\n");
    printf("  },\n");

    /* Window Section */
    printf("  \"window\": {\n");
    printf("    \"method\": \"window\",\n");
    printf("    \"packets_generated\": %u,\n", (unsigned int)res->window.packets_generated);
    printf("    \"packets_received\": %u,\n", (unsigned int)res->window.packets_received);
    printf("    \"packets_lost\": %u,\n", (unsigned int)res->window.packets_lost);
    printf("    \"packets_corrupted\": %u,\n", (unsigned int)res->window.packets_corrupted);
    printf("    \"packets_reordered\": %u,\n", (unsigned int)res->window.packets_reordered);
    printf("    \"errors_detected\": %u,\n", (unsigned int)res->window.errors_detected);
    printf("    \"decoded_message\": ");
    json_print_escaped_string(res->window.decoded_message);
    printf(",\n");
    printf("    \"integrity_match\": %s,\n", res->window.integrity_match ? "true" : "false");
    printf("    \"integrity\": \"%s\",\n", res->window.integrity_status);
    printf("    \"detected_errors\": [");
    for (size_t i = 0; i < res->window.num_detected_errors; i++) {
        printf("\"%s\"%s", res->window.detected_errors[i], (i + 1 < res->window.num_detected_errors) ? ", " : "");
    }
    printf("]\n");
    printf("  }\n");

    printf("}\n");
}
