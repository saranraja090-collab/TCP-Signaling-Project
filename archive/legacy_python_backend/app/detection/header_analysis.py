"""
Header Analysis Module
======================
Examines TCP header fields (Sequence Numbers, Window Sizes) across packet flows
to measure statistical entropy and detect unusual variance.

Planned Workflow:
-----------------
1. Extract sequence number increments across zero-payload packets.
2. Measure distribution and frequency of window size values.
3. Compute Shannon entropy to detect unnatural information packing in headers.
"""


class HeaderAnalyzer:
    """Performs statistical checks and entropy analysis on simulated TCP headers."""

    def __init__(self):
        """Initialize header analyzer."""
        pass

    def calculate_window_entropy(self, packets: list[dict]) -> float:
        """Calculate Shannon entropy of the window size field across the flow.

        Placeholder to be implemented in the detection development step.
        """
        raise NotImplementedError("Header entropy calculation will be implemented in Step 6.")
