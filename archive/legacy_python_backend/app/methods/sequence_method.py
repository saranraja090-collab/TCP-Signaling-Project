"""
Method 1: Sequence-Number-Based Signaling
=========================================
Experimentally explores representing data using the 32-bit TCP Sequence Number field.

Background on TCP Sequence Numbers:
-----------------------------------
- In standard TCP (RFC 793 / RFC 9293), the 32-bit Sequence Number identifies the
  byte order of data in a stream, ensuring reliable, in-order delivery.
- When establishing a connection, each host selects an Initial Sequence Number (ISN).

Experimental Signaling Concept:
-------------------------------
- Instead of using the sequence number solely for byte tracking, this method
  modulates information bits into:
    a) Modulated Initial Sequence Numbers (ISN), or
    b) Relative sequence number increments/deltas between consecutive packets.
- In this simulation, each simulated packet carries 0 bytes of payload, so the
  sequence progression directly conveys the signaling stream.
"""


class SequenceSignalingMethod:
    """Simulates sequence-number-based data modulation and extraction."""

    def __init__(self, base_seq: int = 1000):
        """Initialize the method with an initial base sequence number."""
        self.base_seq = base_seq

    def modulate(self, bits: list[int]) -> list[dict]:
        """Modulate input bits into simulated TCP packet header sequence numbers.

        Placeholder to be implemented in the signaling method development step.
        """
        raise NotImplementedError("Sequence signaling modulation will be implemented in Step 3.")

    def demodulate(self, packets: list[dict]) -> list[int]:
        """Extract signaling bits from simulated TCP packet sequence numbers.

        Placeholder to be implemented in the signaling method development step.
        """
        raise NotImplementedError("Sequence signaling demodulation will be implemented in Step 3.")
