"""
Method 3: Packet Timing-Based Signaling
=======================================
Experimentally explores representing data using inter-packet transmission delays.

Background on Packet Timing:
----------------------------
- In real networks, the time interval between consecutive packets is influenced
  by network congestion, router queueing delays, and transport flow control.

Experimental Signaling Concept:
-------------------------------
- Instead of altering header integer values, this method encodes data into the
  time interval (delta t) between consecutive packet transmissions.
- For example:
    * Short delay (e.g., ~20 ms) represents binary '0'
    * Long delay  (e.g., ~60 ms) represents binary '1'
- The receiver measures arrival timestamps, calculates inter-arrival delays,
  and maps them back to the intended bit sequence based on decision thresholds.
"""


class TimingSignalingMethod:
    """Simulates packet timing (inter-packet delay) modulation and extraction."""

    def __init__(self, delay_zero_ms: float = 20.0, delay_one_ms: float = 60.0):
        """Initialize timing parameters for binary 0 and binary 1."""
        self.delay_zero_ms = delay_zero_ms
        self.delay_one_ms = delay_one_ms

    def modulate(self, bits: list[int]) -> list[dict]:
        """Generate simulated packet timing schedules based on input bits.

        Placeholder to be implemented in the signaling method development step.
        """
        raise NotImplementedError("Timing signaling modulation will be implemented in Step 3.")

    def demodulate(self, packets: list[dict]) -> list[int]:
        """Extract signaling bits from simulated packet inter-arrival timestamps.

        Placeholder to be implemented in the signaling method development step.
        """
        raise NotImplementedError("Timing signaling demodulation will be implemented in Step 3.")
