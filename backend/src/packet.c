/**
 * @file packet.c
 * @brief Implementation of simulated packet initialization and utility functions.
 */

#include "packet.h"
#include <stdio.h>
#include <string.h>

void packet_init(simulated_packet_t *pkt, uint32_t packet_id) {
    if (!pkt) return;

    memset(pkt, 0, sizeof(simulated_packet_t));

    /* Conceptual TCP defaults */
    pkt->src_port = 49152;            /* Dynamic ephemeral client port */
    pkt->dst_port = 80;               /* Target service port (e.g. HTTP) */
    pkt->sequence_number = 10000;     /* Default Initial Sequence Number */
    pkt->acknowledgment_number = 1;
    pkt->window_size = 65535;         /* Standard TCP 64KB window */
    pkt->flags = TCP_FLAG_ACK;

    /* Architectural constraint: 0-byte payload */
    pkt->payload_length = 0;

    /* Simulation metadata */
    pkt->packet_id = packet_id;
    pkt->timestamp_ms = 0.0;
    pkt->inter_packet_gap_ms = 0.0;
    pkt->signaling_method = SIGNALING_METHOD_NONE;
    pkt->signaled_symbol = 0;
}

const char* packet_method_name(signaling_method_t method) {
    switch (method) {
        case SIGNALING_METHOD_SEQUENCE:
            return "Sequence Number";
        case SIGNALING_METHOD_WINDOW:
            return "Window Size";
        case SIGNALING_METHOD_TIMING:
            return "Timing";
        case SIGNALING_METHOD_NONE:
        default:
            return "None / Standard TCP";
    }
}

void packet_format_summary(const simulated_packet_t *pkt, char *buffer, size_t max_len) {
    if (!pkt || !buffer || max_len == 0) return;

    snprintf(buffer, max_len,
             "Pkt#%u [Method: %s] Seq: %u, Win: %u, Delay: %.2f ms, Payload: %u B, Symbol: %u",
             (unsigned int)pkt->packet_id,
             packet_method_name(pkt->signaling_method),
             (unsigned int)pkt->sequence_number,
             (unsigned int)pkt->window_size,
             pkt->inter_packet_gap_ms,
             (unsigned int)pkt->payload_length,
             (unsigned int)pkt->signaled_symbol);
}
