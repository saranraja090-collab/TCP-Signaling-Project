/**
 * @file window_method.h
 * @brief End-to-End Method 2: TCP Window-Size-Based Signaling.
 *
 * Computer Networks Academic Experimental Project:
 * "TCP Header Data Modulation and Signaling"
 *
 * IMPORTANT CONCEPT & SPECIFICATION:
 * 1. Under RFC 793, the 16-bit Window Size field specifies the number of data octets
 *    beginning with the one indicated in the acknowledgment field which the sender
 *    of this segment is willing to accept (flow control).
 * 2. In this experimental simulation, packets carry strictly 0-byte payload (Payload = 0 B).
 * 3. Discrete simulated window sizes represent signaling symbols:
 *      BIT 0 -> WINDOW_VALUE_0 (30000 bytes)
 *      BIT 1 -> WINDOW_VALUE_1 (60000 bytes)
 * 4. The receiver reads packet.window_size and recovers the transmitted bit.
 * 5. Unexpected window values are flagged as errors; unknown values are NOT silently interpreted.
 */

#ifndef TCP_SIGNALING_WINDOW_METHOD_H
#define TCP_SIGNALING_WINDOW_METHOD_H

#include "packet.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Deterministic 16-bit Window Size values for binary signaling */
#define WINDOW_VALUE_0          30000U
#define WINDOW_VALUE_1          60000U
#define WINDOW_VALUE_TOLERANCE  500U

/* Return codes */
#define WIN_SUCCESS                  0
#define WIN_ERR_NULL_PARAM          -1
#define WIN_ERR_BUFFER_OVERFLOW     -2
#define WIN_ERR_INVALID_WINDOW      -3
#define WIN_ERR_EMPTY_INPUT         -4

/**
 * @struct window_method_config_t
 * @brief Configuration parameters for window size signaling.
 */
typedef struct {
    uint16_t window_zero;        /**< Advertised window size for bit 0 (default: 30000) */
    uint16_t window_one;         /**< Advertised window size for bit 1 (default: 60000) */
    uint16_t window_tolerance;   /**< Allowable tolerance for window matching (±500) */
    uint32_t baseline_sequence;  /**< Baseline TCP sequence number for simulated packets */
} window_method_config_t;

/**
 * @brief Initialize window method configuration with defaults.
 *
 * @param config Pointer to configuration struct.
 */
void window_method_init_config(window_method_config_t *config);

/**
 * @brief Get advertised window size for a given bit (0 or 1).
 *
 * @param bit Bit value (0 or 1).
 * @param config Method configuration (or NULL for defaults).
 * @return 16-bit window size (WINDOW_VALUE_0 or WINDOW_VALUE_1).
 */
uint16_t window_get_size_for_bit(uint8_t bit, const window_method_config_t *config);

/**
 * @brief Map a received window size value back to a bit.
 *
 * @param window_size Advertised window size from simulated packet.
 * @param config Method configuration (or NULL for defaults).
 * @param bit_out Output pointer receiving the decoded bit (0 or 1).
 * @return WIN_SUCCESS on valid mapping, WIN_ERR_INVALID_WINDOW on unrecognized value.
 */
int window_get_bit_from_size(uint16_t window_size, const window_method_config_t *config, uint8_t *bit_out);

/**
 * @brief Modulate an array of bits into simulated TCP packets using window sizes.
 *
 * For every bit:
 *  - bit 0: packet.window_size = WINDOW_VALUE_0 (30000)
 *  - bit 1: packet.window_size = WINDOW_VALUE_1 (60000)
 *  - packet.payload_length = 0 (STRICTLY ENFORCED)
 *
 * @param bits Array of 0 and 1 values.
 * @param num_bits Number of bits.
 * @param config Method configuration (or NULL for defaults).
 * @param packets_out Buffer to store generated simulated packets.
 * @param max_packets Capacity of packets_out.
 * @param packets_created Output receiving count of created packets.
 * @return WIN_SUCCESS on success, negative error code on failure.
 */
int window_method_modulate(const uint8_t *bits, size_t num_bits,
                           const window_method_config_t *config,
                           simulated_packet_t *packets_out, size_t max_packets,
                           size_t *packets_created);

/**
 * @brief Demodulate simulated TCP packets back into binary bits via window sizes.
 *
 * Reads packet.window_size:
 *  - 30000 -> bit 0
 *  - 60000 -> bit 1
 * If an unexpected window size is encountered, returns WIN_ERR_INVALID_WINDOW.
 *
 * @param packets Input array of simulated packets.
 * @param num_packets Number of packets.
 * @param config Method configuration (or NULL for defaults).
 * @param bits_out Output buffer for recovered bits.
 * @param max_bits Capacity of bits_out.
 * @param bits_recovered Output receiving count of recovered bits.
 * @param invalid_packet_idx Output receiving index of first invalid packet (if error occurs).
 * @return WIN_SUCCESS on success, WIN_ERR_INVALID_WINDOW on error.
 */
int window_method_demodulate(const simulated_packet_t *packets, size_t num_packets,
                             const window_method_config_t *config,
                             uint8_t *bits_out, size_t max_bits,
                             size_t *bits_recovered,
                             size_t *invalid_packet_idx);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SIGNALING_WINDOW_METHOD_H */
