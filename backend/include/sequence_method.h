/**
 * @file sequence_method.h
 * @brief End-to-End Method 1: TCP Sequence-Number-Based Signaling.
 *
 * Computer Networks Academic Experimental Project:
 * "TCP Header Data Modulation and Signaling"
 *
 * SPECIFICATION & TECHNICAL PRINCIPLES:
 * 1. Sequence numbers in standard TCP (RFC 793 / RFC 9293) maintain byte-stream ordering.
 * 2. In this experimental simulation, packets carry 0-byte payload (Payload = 0 B).
 * 3. Discrete sequence number increments signal individual binary bits:
 *      BIT 0 -> SEQ_INCREMENT_0 (Default: +100)
 *      BIT 1 -> SEQ_INCREMENT_1 (Default: +200)
 * 4. The receiver extracts sequence symbols by measuring delta_seq = current_seq - previous_seq.
 * 5. Unrecognized sequence increments are flagged as invalid deltas.
 */

#ifndef TCP_SIGNALING_SEQUENCE_METHOD_H
#define TCP_SIGNALING_SEQUENCE_METHOD_H

#include "packet.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard sequence signaling parameters */
#define SEQ_DEFAULT_BASE        10000U
#define SEQ_INCREMENT_0         100U
#define SEQ_INCREMENT_1         200U
#define SEQ_DELTA_TOLERANCE     15U

/* Return codes */
#define SEQ_SUCCESS                  0
#define SEQ_ERR_NULL_PARAM          -1
#define SEQ_ERR_BUFFER_OVERFLOW     -2
#define SEQ_ERR_INVALID_DELTA       -3
#define SEQ_ERR_EMPTY_INPUT         -4

/**
 * @struct sequence_method_config_t
 * @brief Configuration parameters for sequence number signaling.
 */
typedef struct {
    uint32_t base_sequence_number;   /**< Initial baseline sequence number (e.g. 10000) */
    uint32_t step_zero;              /**< Sequence increment representing bit 0 (+100) */
    uint32_t step_one;               /**< Sequence increment representing bit 1 (+200) */
    uint32_t delta_tolerance;        /**< Allowable tolerance for delta matching (±15) */
} sequence_method_config_t;

/**
 * @brief Initialize sequence method configuration with defaults.
 *
 * @param config Pointer to configuration struct.
 */
void sequence_method_init_config(sequence_method_config_t *config);

/**
 * @brief Get sequence increment for a given bit (0 or 1).
 *
 * @param bit Bit value (0 or 1).
 * @param config Method configuration (or NULL for defaults).
 * @return Increment value (+100 or +200).
 */
uint32_t sequence_get_increment_for_bit(uint8_t bit, const sequence_method_config_t *config);

/**
 * @brief Map a measured sequence delta back to a bit.
 *
 * @param delta Difference between current and previous sequence number.
 * @param config Method configuration (or NULL for defaults).
 * @param bit_out Output pointer receiving the decoded bit (0 or 1).
 * @return SEQ_SUCCESS if recognized, SEQ_ERR_INVALID_DELTA if unrecognized.
 */
int sequence_get_bit_from_delta(uint32_t delta, const sequence_method_config_t *config, uint8_t *bit_out);

/**
 * @brief Modulate an array of bits into simulated TCP packets using sequence numbers.
 *
 * For each bit:
 *  - Creates one simulated packet with payload_length = 0
 *  - Advances sequence_number by +100 (for bit 0) or +200 (for bit 1)
 *
 * @param bits Array of 0 and 1 values.
 * @param num_bits Number of bits to modulate.
 * @param config Method configuration (or NULL for defaults).
 * @param packets_out Buffer to store generated simulated packets.
 * @param max_packets Capacity of packets_out.
 * @param packets_created Output receiving number of packets created.
 * @return SEQ_SUCCESS on success, negative error code on failure.
 */
int sequence_method_modulate(const uint8_t *bits, size_t num_bits,
                             const sequence_method_config_t *config,
                             simulated_packet_t *packets_out, size_t max_packets,
                             size_t *packets_created);

/**
 * @brief Demodulate simulated TCP packets back into binary bits via sequence offsets.
 *
 * Measures delta_seq = current_seq - previous_seq for each packet.
 * Maps +100 -> bit 0, +200 -> bit 1.
 *
 * @param packets Input array of simulated packets.
 * @param num_packets Number of packets.
 * @param config Method configuration (or NULL for defaults).
 * @param bits_out Output buffer for recovered bits.
 * @param max_bits Capacity of bits_out.
 * @param bits_recovered Output receiving count of recovered bits.
 * @param invalid_packet_idx Output receiving index of first invalid packet (if error occurs).
 * @return SEQ_SUCCESS on success, SEQ_ERR_INVALID_DELTA on unrecognized delta.
 */
int sequence_method_demodulate(const simulated_packet_t *packets, size_t num_packets,
                               const sequence_method_config_t *config,
                               uint8_t *bits_out, size_t max_bits,
                               size_t *bits_recovered,
                               size_t *invalid_packet_idx);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SIGNALING_SEQUENCE_METHOD_H */
