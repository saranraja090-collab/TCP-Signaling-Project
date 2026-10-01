/**
 * @file network_simulator.c
 * @brief Implementation of the In-Memory Network Impairment Simulator.
 */

#include "network_simulator.h"
#include "sequence_method.h"
#include "window_method.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief Fast, portable, deterministic Linear Congruential Generator (LCG).
 * Guarantees identical pseudo-random sequences across all operating systems and compilers.
 */
static uint32_t prng_next(uint32_t *state) {
    *state = (*state * 1103515245U + 12345U) & 0x7FFFFFFFU;
    return *state;
}

static uint32_t prng_range(uint32_t *state, uint32_t max_exclusive) {
    if (max_exclusive == 0) return 0;
    return prng_next(state) % max_exclusive;
}

void network_config_init_default(network_config_t *config) {
    if (!config) return;
    config->loss_percent        = 0;
    config->corruption_percent  = 0;
    config->reorder_percent     = 0;
    config->random_seed         = 12345U;
    config->enable_loss         = 0;
    config->enable_corruption   = 0;
    config->enable_reordering   = 0;
}

void network_stats_init(network_stats_t *stats, const network_config_t *config) {
    if (!stats) return;
    memset(stats, 0, sizeof(*stats));
    if (config) {
        stats->loss_percent       = config->loss_percent;
        stats->corruption_percent = config->corruption_percent;
        stats->reorder_percent    = config->reorder_percent;
        stats->random_seed        = config->random_seed;
    }
}

const char* network_packet_status_str(packet_status_t status) {
    switch (status) {
        case PACKET_STATUS_OK:        return "OK";
        case PACKET_STATUS_CORRUPTED: return "CORRUPTED";
        case PACKET_STATUS_LOST:      return "LOST";
        case PACKET_STATUS_REORDERED: return "REORDERED";
        default:                      return "UNKNOWN";
    }
}

int network_simulator_transmit(const simulated_packet_t *input_packets, size_t input_count,
                               const network_config_t *config,
                               simulated_packet_t *output_packets, size_t max_output,
                               size_t *output_count,
                               network_stats_t *stats,
                               packet_flow_record_t *flow_records, size_t max_flow,
                               size_t *flow_count) {
    if (!output_packets || !output_count || !stats) return -1;
    if (input_count > 0 && !input_packets) return -1;

    network_config_t cfg;
    if (config) {
        cfg = *config;
    } else {
        network_config_init_default(&cfg);
    }

    network_stats_init(stats, &cfg);
    stats->packets_generated = input_count;

    if (input_count == 0) {
        *output_count = 0;
        if (flow_count) *flow_count = 0;
        return 0;
    }

    uint32_t rng_state = cfg.random_seed;
    if (rng_state == 0) rng_state = 12345U;

    /* Temporary working buffer for stage-by-stage pipeline processing */
    simulated_packet_t stage1[input_count];
    packet_status_t status_map[input_count];

    /* Initialize with copies of input packets and enforce 0-byte payload invariant */
    for (size_t i = 0; i < input_count; i++) {
        stage1[i] = input_packets[i];
        stage1[i].payload_length = 0; /* STRICT 0-BYTE PAYLOAD INVARIANT */
        status_map[i] = PACKET_STATUS_OK;
    }

    /* =====================================================================
     * STAGE 1: PACKET CORRUPTION
     * Controlled impairment of signaling field attributes (payload remains 0).
     * ===================================================================== */
    int do_corruption = (cfg.enable_corruption || cfg.corruption_percent > 0);
    if (do_corruption && cfg.corruption_percent > 0) {
        for (size_t i = 0; i < input_count; i++) {
            if (prng_range(&rng_state, 100) < cfg.corruption_percent) {
                status_map[i] = PACKET_STATUS_CORRUPTED;
                stats->packets_corrupted++;

                if (stage1[i].signaling_method == SIGNALING_METHOD_SEQUENCE) {
                    /* Corrupt sequence progression: offset by +350 (valid steps are +100 or +200) */
                    stage1[i].sequence_number += 350U;
                } else if (stage1[i].signaling_method == SIGNALING_METHOD_WINDOW) {
                    /* Corrupt advertised window: set to 45000 (valid values are 30000 or 60000) */
                    stage1[i].window_size = 45000U;
                }

                /* Ensure invariant remains strictly preserved */
                stage1[i].payload_length = 0;
            }
        }
    }

    /* =====================================================================
     * STAGE 2: PACKET LOSS
     * Drop packets from the stream; surviving packets proceed to Stage 3.
     * ===================================================================== */
    simulated_packet_t stage2[input_count];
    packet_status_t status_map2[input_count];
    size_t stage2_count = 0;

    int do_loss = (cfg.enable_loss || cfg.loss_percent > 0);
    for (size_t i = 0; i < input_count; i++) {
        int is_dropped = 0;
        if (do_loss && cfg.loss_percent > 0) {
            if (prng_range(&rng_state, 100) < cfg.loss_percent) {
                is_dropped = 1;
            }
        }

        if (is_dropped) {
            status_map[i] = PACKET_STATUS_LOST;
            stats->packets_lost++;
        } else {
            stage2[stage2_count] = stage1[i];
            status_map2[stage2_count] = status_map[i];
            stage2_count++;
        }
    }

    /* =====================================================================
     * STAGE 3: PACKET REORDERING
     * Permute surviving packets based on reorder_percent.
     * ===================================================================== */
    int do_reorder = (cfg.enable_reordering || cfg.reorder_percent > 0);
    if (do_reorder && cfg.reorder_percent > 0 && stage2_count > 1) {
        for (size_t i = 0; i + 1 < stage2_count; i++) {
            if (prng_range(&rng_state, 100) < cfg.reorder_percent) {
                /* Swap adjacent packets i and i + 1 */
                simulated_packet_t tmp_pkt = stage2[i];
                stage2[i] = stage2[i + 1];
                stage2[i + 1] = tmp_pkt;

                packet_status_t tmp_st = status_map2[i];
                status_map2[i] = PACKET_STATUS_REORDERED;
                status_map2[i + 1] = (tmp_st == PACKET_STATUS_CORRUPTED) ? PACKET_STATUS_CORRUPTED : PACKET_STATUS_REORDERED;

                stats->packets_reordered += 2;
                i++; /* Advance past the swapped pair */
            }
        }
    }

    /* =====================================================================
     * DELIVER PACKETS TO RECEIVER BUFFER & RECORD FLOW TELEMETRY
     * ===================================================================== */
    if (stage2_count > max_output) {
        return -2; /* Output buffer overflow */
    }

    for (size_t i = 0; i < stage2_count; i++) {
        output_packets[i] = stage2[i];
        output_packets[i].payload_length = 0; /* Enforce 0-byte payload invariant */
    }
    *output_count = stage2_count;
    stats->packets_received = stage2_count;

    /* Build flow records for web UI inspector and debugging if requested */
    if (flow_records && max_flow > 0 && flow_count) {
        size_t records_to_write = (input_count < max_flow) ? input_count : max_flow;
        for (size_t i = 0; i < records_to_write; i++) {
            flow_records[i].packet_id       = stage1[i].packet_id;
            flow_records[i].sequence_number = stage1[i].sequence_number;
            flow_records[i].window_size     = stage1[i].window_size;
            flow_records[i].payload_length  = 0;
            flow_records[i].status          = status_map[i];
            flow_records[i].status_str      = network_packet_status_str(status_map[i]);
            flow_records[i].symbol          = stage1[i].signaled_symbol;
        }
        *flow_count = records_to_write;
    } else if (flow_count) {
        *flow_count = 0;
    }

    return 0;
}
