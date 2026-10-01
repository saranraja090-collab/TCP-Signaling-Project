/**
 * @file comparison.h
 * @brief Comparative Analysis Engine for Sequence Number vs Window Size Signaling.
 *
 * Step 5 — Comparative Analysis of Sequence and Window Signaling:
 * Compares Sequence Number Signaling vs Window Size Signaling using:
 * - EXACT SAME original message
 * - EXACT SAME network impairment conditions (loss, corruption, reordering)
 * - EXACT SAME deterministic random seed
 * - STRICT 0-BYTE PAYLOAD INVARIANT on all packets
 *
 * Subjective scoring, rankings, and winners are strictly prohibited.
 */

#ifndef COMPARISON_H
#define COMPARISON_H

#include <stddef.h>
#include <stdint.h>
#include "packet.h"
#include "network_simulator.h"

#define COMP_MAX_TEXT_LEN     2048
#define COMP_MAX_ERRORS       16

typedef struct {
    signaling_method_t method;
    char method_id[32];               /* "sequence" or "window" */
    char method_name[64];             /* "Sequence Number" or "Window Size" */
    size_t packets_generated;
    size_t packets_received;
    size_t packets_lost;
    size_t packets_corrupted;
    size_t packets_reordered;
    size_t payload_bytes;             /* Strictly 0 */
    size_t errors_detected;
    char decoded_message[COMP_MAX_TEXT_LEN];
    int integrity_match;              /* 1 = verified match, 0 = mismatch / failed */
    char integrity_status[32];        /* "verified", "incomplete", "failed" */
    double bit_error_rate;
    char detected_errors[COMP_MAX_ERRORS][64];
    size_t num_detected_errors;
} comparison_method_metrics_t;

typedef struct {
    char original_message[COMP_MAX_TEXT_LEN];
    network_config_t net_cfg;
    size_t payload_bytes;             /* Strictly 0 */
    comparison_method_metrics_t sequence;
    comparison_method_metrics_t window;
    int execution_success;
} comparison_result_t;

/**
 * Execute comparison between Sequence Number and Window Size methods under
 * identical simulation conditions.
 *
 * @param input_msg Plaintext message to test.
 * @param net_cfg Network configuration containing loss, corruption, reordering, and seed.
 * @param out_result Output comparison metrics structure.
 * @return 0 on success, non-zero on failure.
 */
int comparison_execute(const char *input_msg, const network_config_t *net_cfg, comparison_result_t *out_result);

/**
 * Print CLI format comparative analysis report.
 *
 * @param res Comparison result structure.
 */
void comparison_print_cli(const comparison_result_t *res);

/**
 * Print JSON format comparative analysis output for API/frontend integration.
 *
 * @param res Comparison result structure.
 */
void comparison_print_json(const comparison_result_t *res);

#endif /* COMPARISON_H */
