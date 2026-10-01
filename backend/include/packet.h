/**
 * @file packet.h
 * @brief Simulated TCP Packet Data Model for Experimental Signaling.
 *
 * Computer Networks Academic Project:
 * "TCP Header Data Modulation and Signaling"
 *
 * IMPORTANT TECHNICAL PRINCIPLES:
 * 1. TCP header fields have real protocol meanings (RFC 793, RFC 9293).
 * 2. Sequence numbers and window sizes are NOT arbitrary storage fields.
 * 3. This structure models an EXPERIMENTAL signaling/modulation technique
 *    over transport-layer packet attributes.
 * 4. Application payload in this project is ALWAYS ZERO BYTES (Payload = 0 B).
 *    All information is signaled strictly through header/timing modulation.
 * 5. This is a SIMULATED packet representation for in-memory simulation;
 *    it is not a real raw network wire segment.
 */

#ifndef TCP_SIGNALING_PACKET_H
#define TCP_SIGNALING_PACKET_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Signaling method enumeration.
 */
typedef enum {
    SIGNALING_METHOD_NONE     = 0,
    SIGNALING_METHOD_SEQUENCE = 1,  /**< Modulate TCP Sequence Number */
    SIGNALING_METHOD_WINDOW   = 2,  /**< Modulate TCP Advertised Window Size */
    SIGNALING_METHOD_TIMING   = 3   /**< Modulate Inter-Packet Departure Timing */
} signaling_method_t;

/**
 * @brief Standard TCP Header Flags (RFC 793).
 */
#define TCP_FLAG_FIN  (1 << 0)
#define TCP_FLAG_SYN  (1 << 1)
#define TCP_FLAG_RST  (1 << 2)
#define TCP_FLAG_PSH  (1 << 3)
#define TCP_FLAG_ACK  (1 << 4)
#define TCP_FLAG_URG  (1 << 5)

/**
 * @struct simulated_packet_t
 * @brief Represents a single experimental simulated packet in memory.
 */
typedef struct {
    /* =====================================================================
     * 1. CONCEPTUAL TCP HEADER FIELDS (Modeled RFC 793 / 9293 semantics)
     * ===================================================================== */

    /** Conceptual source port (16-bit). */
    uint16_t src_port;

    /** Conceptual destination port (16-bit). */
    uint16_t dst_port;

    /**
     * Conceptual TCP Sequence Number (32-bit).
     * In Method 1 (Sequence Number Signaling), relative offsets or values
     * encode signaling bits. Standard TCP uses this for byte stream ordering.
     */
    uint32_t sequence_number;

    /**
     * Conceptual TCP Acknowledgment Number (32-bit).
     * Identifies next expected sequence number.
     */
    uint32_t acknowledgment_number;

    /**
     * Conceptual TCP Advertised Window Size (16-bit).
     * In Method 2 (Window Size Signaling), discrete window steps encode bits.
     * Standard TCP uses this for receiver buffer flow control throttles.
     */
    uint16_t window_size;

    /**
     * Conceptual TCP Control Flags (8-bit: SYN, ACK, FIN, etc.).
     */
    uint8_t flags;

    /**
     * Application Payload Length in bytes.
     * CRITICAL: In this experimental project, this value is strictly 0.
     * No user data is placed in the payload.
     */
    uint16_t payload_length;

    /* =====================================================================
     * 2. SIMULATION & INSTRUMENTATION METADATA (Simulation Only)
     * These fields do NOT exist on the physical wire; they exist purely
     * to instrument, measure, and analyze the simulation experiments.
     * ===================================================================== */

    /** Simulation packet sequential identifier (0, 1, 2, ...). */
    uint32_t packet_id;

    /** Simulated departure timestamp in milliseconds (monotonically increasing). */
    double timestamp_ms;

    /**
     * Inter-packet gap (delta t) relative to previous packet in milliseconds.
     * In Method 3 (Timing Signaling), delta t encodes bits (e.g., T_short vs T_long).
     */
    double inter_packet_gap_ms;

    /** The signaling method under test for this packet. */
    signaling_method_t signaling_method;

    /** The discrete symbol or bit (0 or 1) signaled by this packet. */
    uint8_t signaled_symbol;

} simulated_packet_t;

/**
 * @brief Initialize a simulated packet with baseline defaults.
 *
 * Sets payload_length to 0, flags to ACK, and resets metadata.
 *
 * @param pkt Pointer to simulated packet structure.
 * @param packet_id Numerical ID for the packet.
 */
void packet_init(simulated_packet_t *pkt, uint32_t packet_id);

/**
 * @brief Format a packet's summary into a human-readable string.
 *
 * @param pkt Pointer to packet.
 * @param buffer Output char buffer.
 * @param max_len Maximum length of buffer.
 */
void packet_format_summary(const simulated_packet_t *pkt, char *buffer, size_t max_len);

/**
 * @brief Get human-readable name of signaling method.
 *
 * @param method Signaling method enum value.
 * @return String description ("Sequence Number", "Window Size", "Timing", or "None").
 */
const char* packet_method_name(signaling_method_t method);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SIGNALING_PACKET_H */
