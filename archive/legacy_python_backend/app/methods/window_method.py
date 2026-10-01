"""
Method 2: TCP Window-Size-Based Signaling
=========================================
Experimentally explores representing data using the 16-bit TCP Window Size field.

Background on TCP Window Size:
------------------------------
- In standard TCP, the 16-bit Window Size field (also called Advertised Window)
  implements end-to-end flow control. It tells the sender how many bytes of buffer
  space the receiver currently has available to prevent receiver buffer overflow.

Experimental Signaling Concept:
-------------------------------
- Instead of reflecting real receive buffer capacity, the sender sets the
  window size to values corresponding to discrete symbols or bit patterns.
- For example, specific window size values or modulations within a plausible
  range (e.g., multiples of a step size) represent encoded chunks of bits.
- Because payload is 0 bytes, no real data consumes the receive buffer.
"""


class WindowSignalingMethod:
    """Simulates TCP window-size-based data modulation and extraction."""

    def __init__(self, step_size: int = 64, base_window: int = 8192):
        """Initialize the method with window calculation parameters."""
        self.step_size = step_size
        self.base_window = base_window

    def modulate(self, bits: list[int]) -> list[dict]:
        """Modulate input bits into simulated TCP packet window size fields.

        Placeholder to be implemented in the signaling method development step.
        """
        raise NotImplementedError("Window signaling modulation will be implemented in Step 3.")

    def demodulate(self, packets: list[dict]) -> list[int]:
        """Extract signaling bits from simulated TCP packet window sizes.

        Placeholder to be implemented in the signaling method development step.
        """
        raise NotImplementedError("Window signaling demodulation will be implemented in Step 3.")
