/**
 * @file main.c
 * @brief Main entry point for the C Networking Backend Engine.
 *
 * Computer Networks Academic Project:
 * "TCP Header Data Modulation and Signaling"
 *
 * Architecture & Step 4 Capabilities:
 * 1. Method 1: Sequence Number Signaling (delta_seq = +100 / +200).
 * 2. Method 2: Window Size Signaling (window_size = 30000 / 60000 B).
 * 3. Step 4 Simulated Network Layer: In-memory simulation of packet corruption,
 *    loss, and reordering.
 * 4. STRICT ZERO-PAYLOAD INVARIANT: All simulated packets carry strictly 0 bytes of payload.
 * 5. Deterministic random seed and comprehensive metrics reporting.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packet.h"
#include "encoder.h"
#include "error_control.h"
#include "sequence_method.h"
#include "window_method.h"
#include "network_simulator.h"
#include "comparison.h"
#include "decoder.h"
#include "verification.h"
#include "unit_tests.h"

#define MAX_BITS_BUFFER    8192
#define MAX_PACKET_BUFFER  8192
#define MAX_TEXT_BUFFER    2048
#define MAX_FLOW_RECORDS   256

static void print_banner(void) {
    printf("==================================================\n");
    printf("TCP HEADER DATA MODULATION AND SIGNALING\n");
    printf("C BACKEND ENGINE\n");
    printf("==================================================\n");
    printf("\n");
    printf("Backend Status: READY\n");
    printf("\n");
    printf("Available methods:\n");
    printf("\n");
    printf("1. Sequence Number\n");
    printf("2. Window Size\n");
    printf("\n");
    printf("Execution Mode:\n");
    printf("In-Memory Simulation (with Network Impairment Layer)\n");
    printf("\n");
    printf("Payload:\n");
    printf("0 bytes\n");
    printf("\n");
    printf("==================================================\n");
}

static void print_json_status(void) {
    printf("{\n");
    printf("  \"backend\": \"C\",\n");
    printf("  \"status\": \"READY\",\n");
    printf("  \"execution_mode\": \"In-Memory Simulation\",\n");
    printf("  \"payload_bytes\": 0,\n");
    printf("  \"methods\": [\n");
    printf("    {\"id\": \"sequence\", \"name\": \"Sequence Number\", \"field\": \"TCP sequence_number (32-bit)\", \"status\": \"ACTIVE\"},\n");
    printf("    {\"id\": \"window\", \"name\": \"Window Size\", \"field\": \"TCP window_size (16-bit)\", \"status\": \"ACTIVE\"}\n");
    printf("  ]\n");
    printf("}\n");
}

static void print_json_escaped_string(const char *str) {
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

static void format_bits_display(const uint8_t *bits, size_t num_bits) {
    if (num_bits == 0) {
        printf("[No bits recovered]\n");
        return;
    }
    if (num_bits <= 128) {
        for (size_t i = 0; i < num_bits; i++) {
            putchar(bits[i] ? '1' : '0');
            if ((i + 1) % 8 == 0 && (i + 1) < num_bits) putchar(' ');
        }
        putchar('\n');
    } else {
        for (size_t i = 0; i < 64; i++) {
            putchar(bits[i] ? '1' : '0');
            if ((i + 1) % 8 == 0) putchar(' ');
        }
        printf("... ");
        for (size_t i = num_bits - 32; i < num_bits; i++) {
            putchar(bits[i] ? '1' : '0');
            if ((i + 1) % 8 == 0 && (i + 1) < num_bits) putchar(' ');
        }
        putchar('\n');
    }
}

static void print_sequence_cli_report(const char *input_msg,
                                      const uint8_t *bits, size_t num_bits,
                                      const simulated_packet_t *packets, size_t num_packets,
                                      const uint8_t *rec_bits, size_t num_rec,
                                      const char *decoded_msg, int is_match,
                                      const network_stats_t *stats,
                                      const char *detected_errors) {
    printf("========================================\n");
    printf("SEQUENCE NUMBER SIGNALING\n");
    printf("========================================\n\n");

    printf("Original Message:\n%s\n\n", input_msg);

    if (stats->loss_percent > 0 || stats->corruption_percent > 0 || stats->reorder_percent > 0) {
        printf("Network Conditions:\n");
        if (stats->loss_percent > 0) printf("Packet Loss: %u%%\n", stats->loss_percent);
        if (stats->corruption_percent > 0) printf("Packet Corruption: %u%%\n", stats->corruption_percent);
        if (stats->reorder_percent > 0) printf("Packet Reordering: %u%%\n", stats->reorder_percent);
        printf("Random Seed: %u\n\n", stats->random_seed);

        printf("Packets Generated: %u\n", (unsigned int)stats->packets_generated);
        printf("Packets Received: %u\n", (unsigned int)stats->packets_received);
        printf("Packets Lost: %u\n", (unsigned int)stats->packets_lost);
        printf("Packets Corrupted: %u\n", (unsigned int)stats->packets_corrupted);
        printf("Packets Reordered: %u\n", (unsigned int)stats->packets_reordered);
    } else {
        printf("Encoded Bits:\n");
        format_bits_display(bits, num_bits);
        printf("\n");

        printf("Number of Bits:\n%u\n\n", (unsigned int)num_bits);
        printf("Number of Packets:\n%u\n\n", (unsigned int)num_packets);
        printf("Payload Per Packet:\n0 bytes\n\n");

        printf("Sequence Mapping:\n");
        printf("0 -> +100\n");
        printf("1 -> +200\n\n");
    }

    if (stats->errors_detected > 0 && detected_errors && detected_errors[0] != '\0') {
        printf("\nDetected Errors:\n%s\n", detected_errors);
    }

    printf("\n----------------------------------------\n");
    printf("PACKET STREAM\n");
    printf("----------------------------------------\n\n");

    if (num_packets == 0) {
        printf("[No packets reached receiver due to 100%% packet loss]\n\n");
    } else if (num_packets <= 10) {
        for (size_t i = 0; i < num_packets; i++) {
            printf("Packet %u:\n", (unsigned int)packets[i].packet_id);
            printf("SEQ = %u\n", (unsigned int)packets[i].sequence_number);
            printf("Payload = %u\n\n", (unsigned int)packets[i].payload_length);
        }
    } else {
        for (size_t i = 0; i < 4; i++) {
            printf("Packet %u:\n", (unsigned int)packets[i].packet_id);
            printf("SEQ = %u\n", (unsigned int)packets[i].sequence_number);
            printf("Payload = %u\n\n", (unsigned int)packets[i].payload_length);
        }
        printf("... [%u intermediate simulated packets carrying 0 bytes payload] ...\n\n",
               (unsigned int)(num_packets - 6));
        for (size_t i = num_packets - 2; i < num_packets; i++) {
            printf("Packet %u:\n", (unsigned int)packets[i].packet_id);
            printf("SEQ = %u\n", (unsigned int)packets[i].sequence_number);
            printf("Payload = %u\n\n", (unsigned int)packets[i].payload_length);
        }
    }

    printf("----------------------------------------\n");
    printf("RECEIVER\n");
    printf("----------------------------------------\n\n");

    printf("Decoded Bits:\n");
    format_bits_display(rec_bits, num_rec);
    printf("\n");

    printf("Decoded Message:\n%s\n\n", decoded_msg);

    printf("Integrity:\n%s\n\n", is_match ? "VERIFIED" : (stats->packets_lost > 0) ? "FAILED / INCOMPLETE" : "FAILED");
    printf("========================================\n");
}

static void print_window_cli_report(const char *input_msg,
                                    size_t num_bits,
                                    const simulated_packet_t *packets, size_t num_packets,
                                    const char *decoded_msg, int is_match,
                                    const network_stats_t *stats,
                                    const char *detected_errors) {
    printf("========================================\n");
    printf("WINDOW SIZE SIGNALING\n");
    printf("========================================\n\n");

    printf("Original Message:\n%s\n\n", input_msg);

    if (stats->loss_percent > 0 || stats->corruption_percent > 0 || stats->reorder_percent > 0) {
        printf("Network Conditions:\n");
        if (stats->loss_percent > 0) printf("Packet Loss: %u%%\n", stats->loss_percent);
        if (stats->corruption_percent > 0) printf("Packet Corruption: %u%%\n", stats->corruption_percent);
        if (stats->reorder_percent > 0) printf("Packet Reordering: %u%%\n", stats->reorder_percent);
        printf("Random Seed: %u\n\n", stats->random_seed);

        printf("Packets Generated: %u\n", (unsigned int)stats->packets_generated);
        printf("Packets Received: %u\n", (unsigned int)stats->packets_received);
        printf("Packets Lost: %u\n", (unsigned int)stats->packets_lost);
        printf("Packets Corrupted: %u\n", (unsigned int)stats->packets_corrupted);
        printf("Packets Reordered: %u\n", (unsigned int)stats->packets_reordered);
    } else {
        printf("Number of Bits:\n%u\n\n", (unsigned int)num_bits);
        printf("Number of Packets:\n%u\n\n", (unsigned int)num_packets);
        printf("Payload Per Packet:\n0 bytes\n\n");

        printf("Window Mapping:\n");
        printf("0 -> %u\n", (unsigned int)WINDOW_VALUE_0);
        printf("1 -> %u\n\n", (unsigned int)WINDOW_VALUE_1);
    }

    if (stats->errors_detected > 0 && detected_errors && detected_errors[0] != '\0') {
        printf("\nDetected Errors:\n%s\n", detected_errors);
    }

    printf("\n----------------------------------------\n");
    printf("PACKET STREAM\n");
    printf("----------------------------------------\n\n");

    if (num_packets == 0) {
        printf("[No packets reached receiver due to 100%% packet loss]\n\n");
    } else if (num_packets <= 10) {
        for (size_t i = 0; i < num_packets; i++) {
            printf("Packet %u:\n", (unsigned int)packets[i].packet_id);
            printf("SEQ = %u\n", (unsigned int)packets[i].sequence_number);
            printf("WINDOW = %u\n", (unsigned int)packets[i].window_size);
            printf("Payload = %u\n\n", (unsigned int)packets[i].payload_length);
        }
    } else {
        for (size_t i = 0; i < 4; i++) {
            printf("Packet %u:\n", (unsigned int)packets[i].packet_id);
            printf("SEQ = %u\n", (unsigned int)packets[i].sequence_number);
            printf("WINDOW = %u\n", (unsigned int)packets[i].window_size);
            printf("Payload = %u\n\n", (unsigned int)packets[i].payload_length);
        }
        printf("... [%u intermediate simulated packets carrying 0 bytes payload] ...\n\n",
               (unsigned int)(num_packets - 6));
        for (size_t i = num_packets - 2; i < num_packets; i++) {
            printf("Packet %u:\n", (unsigned int)packets[i].packet_id);
            printf("SEQ = %u\n", (unsigned int)packets[i].sequence_number);
            printf("WINDOW = %u\n", (unsigned int)packets[i].window_size);
            printf("Payload = %u\n\n", (unsigned int)packets[i].payload_length);
        }
    }

    printf("----------------------------------------\n");
    printf("RECEIVER\n");
    printf("----------------------------------------\n\n");

    printf("Decoded Message:\n%s\n\n", decoded_msg);

    printf("Integrity:\n%s\n\n", is_match ? "VERIFIED" : (stats->packets_lost > 0) ? "FAILED / INCOMPLETE" : "FAILED");
    printf("========================================\n");
}

static int run_experiment(const char *method_name, const char *input_msg,
                          const network_config_t *net_cfg, int json_mode) {
    if (!method_name || !input_msg) return -1;

    signaling_method_t method = SIGNALING_METHOD_NONE;
    if (strcmp(method_name, "sequence") == 0 || strcmp(method_name, "1") == 0) {
        method = SIGNALING_METHOD_SEQUENCE;
    } else if (strcmp(method_name, "window") == 0 || strcmp(method_name, "2") == 0) {
        method = SIGNALING_METHOD_WINDOW;
    } else {
        if (json_mode) {
            printf("{\"status\": \"error\", \"message\": \"Unknown or removed signaling method '%s'. Supported methods: sequence, window.\"}\n", method_name);
        } else {
            fprintf(stderr, "Error: Unknown or removed signaling method '%s'. Supported: sequence, window.\n", method_name);
        }
        return -1;
    }

    /* Step 1: Binary Text Encoding */
    uint8_t bits[MAX_BITS_BUFFER];
    size_t num_bits = 0;
    if (encoder_text_to_bits(input_msg, bits, MAX_BITS_BUFFER, &num_bits) != 0) {
        if (json_mode) printf("{\"status\": \"error\", \"message\": \"Message exceeds bit buffer capacity.\"}\n");
        return -1;
    }

    /* Step 2: Selected Signaling Modulation (Packet Generation) */
    static simulated_packet_t tx_packets[MAX_PACKET_BUFFER];
    size_t tx_count = 0;

    if (method == SIGNALING_METHOD_SEQUENCE) {
        int mod_res = sequence_method_modulate(bits, num_bits, NULL, tx_packets, MAX_PACKET_BUFFER, &tx_count);
        if (mod_res != SEQ_SUCCESS) {
            if (json_mode) printf("{\"status\": \"error\", \"message\": \"Sequence modulation failed.\"}\n");
            return -1;
        }
    } else if (method == SIGNALING_METHOD_WINDOW) {
        int mod_res = window_method_modulate(bits, num_bits, NULL, tx_packets, MAX_PACKET_BUFFER, &tx_count);
        if (mod_res != WIN_SUCCESS) {
            if (json_mode) printf("{\"status\": \"error\", \"message\": \"Window modulation failed.\"}\n");
            return -1;
        }
    }

    /* Verify 0-byte payload invariant on all generated packets */
    for (size_t i = 0; i < tx_count; i++) {
        tx_packets[i].payload_length = 0;
    }

    /* Step 3: Simulated Network Layer (Impairment Pipeline) */
    static simulated_packet_t rx_packets[MAX_PACKET_BUFFER];
    size_t rx_count = 0;
    network_stats_t net_stats;
    static packet_flow_record_t flow_records[MAX_FLOW_RECORDS];
    size_t flow_count = 0;

    int sim_res = network_simulator_transmit(tx_packets, tx_count, net_cfg,
                                            rx_packets, MAX_PACKET_BUFFER, &rx_count,
                                            &net_stats, flow_records, MAX_FLOW_RECORDS, &flow_count);
    if (sim_res != 0) {
        if (json_mode) printf("{\"status\": \"error\", \"message\": \"Network simulation pipeline failed.\"}\n");
        return -1;
    }

    /* Verify 0-byte payload invariant on all delivered packets */
    for (size_t i = 0; i < rx_count; i++) {
        rx_packets[i].payload_length = 0;
    }

    /* Step 4: Receiver Demodulation & Anomaly/Disruption Detection */
    uint8_t recovered_bits[MAX_BITS_BUFFER];
    size_t bits_recovered = 0;
    size_t invalid_idx = 0;
    int order_error_detected = 0;
    int loss_disruption_detected = 0;
    int corruption_detected = 0;
    char error_list_str[512] = "";

    /* 4A. Receiver Packet Order Detection */
    if (rx_count > 1) {
        for (size_t i = 0; i + 1 < rx_count; i++) {
            if (method == SIGNALING_METHOD_SEQUENCE) {
                if (rx_packets[i + 1].sequence_number <= rx_packets[i].sequence_number ||
                    rx_packets[i + 1].packet_id != rx_packets[i].packet_id + 1) {
                    order_error_detected = 1;
                    break;
                }
            } else if (method == SIGNALING_METHOD_WINDOW) {
                if (rx_packets[i + 1].packet_id != rx_packets[i].packet_id + 1) {
                    order_error_detected = 1;
                    break;
                }
            }
        }
    }

    /* 4B. Receiver Packet Loss Detection */
    if (net_stats.packets_lost > 0 || rx_count < tx_count) {
        loss_disruption_detected = 1;
    }

    /* 4C. Demodulation */
    int dem_status = 0;
    if (method == SIGNALING_METHOD_SEQUENCE) {
        dem_status = sequence_method_demodulate(rx_packets, rx_count, NULL, recovered_bits, MAX_BITS_BUFFER, &bits_recovered, &invalid_idx);
        if (dem_status != SEQ_SUCCESS) {
            corruption_detected = 1;
        }
    } else if (method == SIGNALING_METHOD_WINDOW) {
        dem_status = window_method_demodulate(rx_packets, rx_count, NULL, recovered_bits, MAX_BITS_BUFFER, &bits_recovered, &invalid_idx);
        if (dem_status != WIN_SUCCESS) {
            corruption_detected = 1;
        }
    }

    /* Compile detected errors for reporting */
    if (corruption_detected) {
        net_stats.errors_detected++;
        if (method == SIGNALING_METHOD_SEQUENCE) {
            strcat(error_list_str, "INVALID_SEQUENCE_DELTA");
        } else {
            strcat(error_list_str, "INVALID_WINDOW_VALUE");
        }
    }
    if (order_error_detected) {
        net_stats.errors_detected++;
        if (error_list_str[0] != '\0') strcat(error_list_str, ", ");
        strcat(error_list_str, "PACKET_ORDER_ERROR");
    }
    if (loss_disruption_detected) {
        net_stats.errors_detected += net_stats.packets_lost;
        if (error_list_str[0] != '\0') strcat(error_list_str, ", ");
        strcat(error_list_str, "PACKET_LOSS_DISRUPTION");
    }

    /* Step 5: UTF-8 Reconstruction */
    char reconstructed_text[MAX_TEXT_BUFFER];
    size_t chars_decoded = 0;
    if (bits_recovered > 0) {
        decoder_bits_to_text(recovered_bits, bits_recovered, reconstructed_text, MAX_TEXT_BUFFER, &chars_decoded);
    } else {
        reconstructed_text[0] = '\0';
    }

    /* Step 6: Integrity Verification & BER Analysis */
    int is_match = 0;
    if (net_stats.errors_detected == 0 && dem_status == 0 && bits_recovered == num_bits) {
        is_match = verification_check_match(input_msg, reconstructed_text);
    }

    double ber = 0.0;
    if (is_match) {
        ber = 0.0;
    } else if (bits_recovered == num_bits) {
        ber = verification_calculate_ber(bits, recovered_bits, num_bits);
    } else {
        /* Distinguish packet loss/disruption from pure bit errors */
        ber = (bits_recovered == 0) ? 1.0 : (double)(num_bits - bits_recovered) / (double)num_bits;
    }

    const char *status_str = is_match ? "success" : (loss_disruption_detected && rx_count > 0) ? "incomplete" : "failed";
    const char *integrity_str = is_match ? "verified" : (loss_disruption_detected && rx_count > 0) ? "incomplete" : "failed";

    if (json_mode) {
        printf("{\n");
        printf("  \"status\": \"%s\",\n", status_str);
        printf("  \"method\": \"%s\",\n", (method == SIGNALING_METHOD_SEQUENCE) ? "sequence" : "window");
        printf("  \"method_name\": \"%s\",\n", packet_method_name(method));
        printf("  \"method_id\": \"%s\",\n", method_name);
        printf("  \"execution_mode\": \"In-Memory Simulation\",\n");
        printf("  \"payload_bytes\": 0,\n");
        printf("  \"original_message\": ");
        print_json_escaped_string(input_msg);
        printf(",\n");
        printf("  \"decoded_message\": ");
        print_json_escaped_string(reconstructed_text);
        printf(",\n");
        printf("  \"reconstructed_message\": ");
        print_json_escaped_string(reconstructed_text);
        printf(",\n");

        printf("  \"packet_statistics\": {\n");
        printf("    \"generated\": %u,\n", (unsigned int)net_stats.packets_generated);
        printf("    \"received\": %u,\n", (unsigned int)net_stats.packets_received);
        printf("    \"lost\": %u,\n", (unsigned int)net_stats.packets_lost);
        printf("    \"corrupted\": %u,\n", (unsigned int)net_stats.packets_corrupted);
        printf("    \"reordered\": %u\n", (unsigned int)net_stats.packets_reordered);
        printf("  },\n");

        printf("  \"network_conditions\": {\n");
        printf("    \"loss_percent\": %u,\n", net_stats.loss_percent);
        printf("    \"corruption_percent\": %u,\n", net_stats.corruption_percent);
        printf("    \"reorder_percent\": %u,\n", net_stats.reorder_percent);
        printf("    \"random_seed\": %u\n", net_stats.random_seed);
        printf("  },\n");

        /* Backward compatibility fields */
        printf("  \"packet_count\": %u,\n", (unsigned int)net_stats.packets_received);
        printf("  \"packets_count\": %u,\n", (unsigned int)net_stats.packets_received);
        printf("  \"bits_encoded\": %u,\n", (unsigned int)num_bits);
        printf("  \"bit_error_rate\": %.4f,\n", ber);
        printf("  \"integrity_match\": %s,\n", is_match ? "true" : "false");
        printf("  \"integrity\": \"%s\",\n", integrity_str);
        printf("  \"errors_detected\": %u,\n", (unsigned int)net_stats.errors_detected);

        printf("  \"detected_errors\": [");
        if (corruption_detected) {
            printf("\"%s\"", (method == SIGNALING_METHOD_SEQUENCE) ? "INVALID_SEQUENCE_DELTA" : "INVALID_WINDOW_VALUE");
        }
        if (order_error_detected) {
            printf("%s\"PACKET_ORDER_ERROR\"", corruption_detected ? ", " : "");
        }
        if (loss_disruption_detected) {
            printf("%s\"PACKET_LOSS_DISRUPTION\"", (corruption_detected || order_error_detected) ? ", " : "");
        }
        printf("],\n");

        if (method == SIGNALING_METHOD_SEQUENCE) {
            printf("  \"sequence_mapping\": {\n");
            printf("    \"bit_0\": \"+100\",\n");
            printf("    \"bit_1\": \"+200\",\n");
            printf("    \"base_seq\": %u\n", (unsigned int)SEQ_DEFAULT_BASE);
            printf("  },\n");
        } else if (method == SIGNALING_METHOD_WINDOW) {
            printf("  \"window_mapping\": {\n");
            printf("    \"bit_0\": %u,\n", (unsigned int)WINDOW_VALUE_0);
            printf("    \"bit_1\": %u\n", (unsigned int)WINDOW_VALUE_1);
            printf("  },\n");
        }

        /* Sample packets */
        printf("  \"sample_packets\": [\n");
        size_t samples_to_show = (rx_count < 5) ? rx_count : 5;
        for (size_t s = 0; s < samples_to_show; s++) {
            printf("    {\"id\": %u, \"seq\": %u, \"win\": %u, \"delay_ms\": %.1f, \"payload_b\": 0, \"symbol\": %u}%s\n",
                   (unsigned int)rx_packets[s].packet_id,
                   (unsigned int)rx_packets[s].sequence_number,
                   (unsigned int)rx_packets[s].window_size,
                   rx_packets[s].inter_packet_gap_ms,
                   (unsigned int)rx_packets[s].signaled_symbol,
                   (s + 1 < samples_to_show) ? "," : "");
        }
        printf("  ],\n");

        /* Packet flow visualization records */
        printf("  \"packet_flow\": [\n");
        size_t flows_to_show = (flow_count < 10) ? flow_count : 10;
        for (size_t f = 0; f < flows_to_show; f++) {
            printf("    {\"id\": %u, \"seq\": %u, \"win\": %u, \"payload_b\": 0, \"status\": \"%s\", \"symbol\": %u}%s\n",
                   (unsigned int)flow_records[f].packet_id,
                   (unsigned int)flow_records[f].sequence_number,
                   (unsigned int)flow_records[f].window_size,
                   flow_records[f].status_str,
                   (unsigned int)flow_records[f].symbol,
                   (f + 1 < flows_to_show) ? "," : "");
        }
        printf("  ]\n");
        printf("}\n");
    } else {
        if (method == SIGNALING_METHOD_SEQUENCE) {
            print_sequence_cli_report(input_msg, bits, num_bits, rx_packets, rx_count,
                                      recovered_bits, bits_recovered, reconstructed_text, is_match,
                                      &net_stats, error_list_str);
        } else if (method == SIGNALING_METHOD_WINDOW) {
            print_window_cli_report(input_msg, num_bits, rx_packets, rx_count,
                                    reconstructed_text, is_match,
                                    &net_stats, error_list_str);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int json_mode = 0;
    const char *method = NULL;
    const char *msg = NULL;
    network_config_t net_cfg;
    network_config_init_default(&net_cfg);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0) {
            json_mode = 1;
        } else if (strcmp(argv[i], "--test") == 0 || strcmp(argv[i], "-t") == 0) {
            return run_backend_unit_tests();
        } else if (strcmp(argv[i], "--status") == 0) {
            print_json_status();
            return 0;
        } else if ((strcmp(argv[i], "--method") == 0 || strcmp(argv[i], "-m") == 0) && (i + 1 < argc)) {
            method = argv[++i];
        } else if ((strcmp(argv[i], "--msg") == 0 || strcmp(argv[i], "--message") == 0 || strcmp(argv[i], "-d") == 0) && (i + 1 < argc)) {
            msg = argv[++i];
        } else if (strcmp(argv[i], "--loss") == 0 && (i + 1 < argc)) {
            net_cfg.loss_percent = (uint32_t)atoi(argv[++i]);
            net_cfg.enable_loss = 1;
        } else if (strcmp(argv[i], "--corrupt") == 0 && (i + 1 < argc)) {
            net_cfg.corruption_percent = (uint32_t)atoi(argv[++i]);
            net_cfg.enable_corruption = 1;
        } else if (strcmp(argv[i], "--reorder") == 0 && (i + 1 < argc)) {
            net_cfg.reorder_percent = (uint32_t)atoi(argv[++i]);
            net_cfg.enable_reordering = 1;
        } else if (strcmp(argv[i], "--seed") == 0 && (i + 1 < argc)) {
            net_cfg.random_seed = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_banner();
            printf("\nUsage:\n");
            printf("  backend_engine.exe                     # Display ready banner\n");
            printf("  backend_engine.exe --status / --json   # Output JSON status for Flask frontend\n");
            printf("  backend_engine.exe --test              # Run built-in unit tests\n");
            printf("  backend_engine.exe -m sequence -d \"<text>\" [--loss P] [--corrupt P] [--reorder P] [--seed S] [--json]\n");
            printf("  backend_engine.exe -m window -d \"<text>\" [--loss P] [--corrupt P] [--reorder P] [--seed S] [--json]\n");
            printf("  backend_engine.exe -m compare -d \"<text>\" [--loss P] [--corrupt P] [--reorder P] [--seed S] [--json]\n");
            return 0;
        }
    }

    if (method && msg) {
        if (strcmp(method, "compare") == 0 || strcmp(method, "comparison") == 0) {
            comparison_result_t comp_res;
            if (comparison_execute(msg, &net_cfg, &comp_res) != 0) {
                if (json_mode) printf("{\"status\": \"error\", \"message\": \"Comparative analysis execution failed.\"}\n");
                return -1;
            }
            if (json_mode) {
                comparison_print_json(&comp_res);
            } else {
                comparison_print_cli(&comp_res);
            }
            return 0;
        }
        return run_experiment(method, msg, &net_cfg, json_mode);
    }

    if (json_mode) {
        print_json_status();
        return 0;
    }

    /* Default banner mode required by specification */
    print_banner();
    return 0;
}
