/**
 * @file timing_method.h
 * @brief Experimental Method 3: TCP Packet Timing-Based Signaling.
 *
 * Computer Networks Academic Concept:
 * Modulates the inter-packet departure delay (delta t) between consecutive packets.
 * For example:
 *   delta t = T_short (e.g. 15 ms) -> bit 0
 *   delta t = T_long  (e.g. 45 ms) -> bit 1
 * Packets carry 0-byte payload.
 */

#ifndef TCP_SIGNALING_TIMING_METHOD_H
#define TCP_SIGNALING_TIMING_METHOD_H

#include "packet.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct timing_method_config_t
 * @brief Configuration parameters for timing-based signaling.
 */
typedef struct {
    double delay_zero_ms; /**< Inter-packet delay for bit 0 in milliseconds (e.g. 15.0) */
    double delay_one_ms;  /**< Inter-packet delay for bit 1 in milliseconds (e.g. 45.0) */
} timing_method_config_t;

/**
 * @brief Initialize timing method configuration with defaults.
 *
 * @param config Pointer to configuration struct.
 */
void timing_method_init_config(timing_method_config_t *config);

/**
 * @brief Modulate an array of bits into simulated TCP packets using timing delays.
 *
 * @param bits Array of 0 and 1 values.
 * @param num_bits Number of bits.
 * @param config Method configuration (or NULL for defaults).
 * @param packets_out Buffer to store generated simulated packets.
 * @param max_packets Capacity of packets_out.
 * @param packets_created Output receiving count of created packets.
 * @return 0 on success, -1 on error.
 */
int timing_method_modulate(const uint8_t *bits, size_t num_bits,
                           const timing_method_config_t *config,
                           simulated_packet_t *packets_out, size_t max_packets,
                           size_t *packets_created);

/**
 * @brief Demodulate simulated TCP packets back into binary bits via inter-packet delays.
 *
 * @param packets Input array of simulated packets.
 * @param num_packets Number of packets.
 * @param config Method configuration (or NULL for defaults).
 * @param bits_out Output buffer for recovered bits.
 * @param max_bits Capacity of bits_out.
 * @param bits_recovered Output receiving count of recovered bits.
 * @return 0 on success, -1 on error.
 */
int timing_method_demodulate(const simulated_packet_t *packets, size_t num_packets,
                             const timing_method_config_t *config,
                             uint8_t *bits_out, size_t max_bits,
                             size_t *bits_recovered);

#ifdef __cplusplus
}
#endif

#endif /* TCP_SIGNALING_TIMING_METHOD_H */
