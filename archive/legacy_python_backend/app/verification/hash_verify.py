"""
Verification & Integrity Module
===============================
Provides cryptographic hash verification (SHA-256) and Bit Error Rate (BER)
calculation to confirm that the recovered message exactly matches the original.

Why Verification is Important in Computer Networks:
---------------------------------------------------
- In physical and simulated channels, noise, timing jitter, or dropped packets
  can cause bit flips or missing symbols.
- By comparing cryptographic hashes (like SHA-256) of the original plaintext
  against the recovered plaintext, we mathematically prove complete message fidelity.
"""

import hashlib


class IntegrityVerifier:
    """Verifies message integrity and calculates error metrics between transmission endpoints."""

    @staticmethod
    def compute_sha256(data: str) -> str:
        """Compute the hexadecimal SHA-256 hash of a text string.

        Args:
            data: The input string message.

        Returns:
            A 64-character hexadecimal hash string.
        """
        encoded_bytes = data.encode("utf-8")
        return hashlib.sha256(encoded_bytes).hexdigest()

    @staticmethod
    def verify_exact_match(original: str, recovered: str) -> bool:
        """Check whether the original message and recovered message match identically.

        Args:
            original: The original sent string.
            recovered: The reconstructed received string.

        Returns:
            True if strings match exactly, False otherwise.
        """
        return original == recovered

    @staticmethod
    def calculate_bit_error_rate(original_bits: list[int], recovered_bits: list[int]) -> float:
        """Calculate the Bit Error Rate (BER) between original and recovered bit sequences.

        BER = (Number of mismatched bits) / (Total number of bits)

        Args:
            original_bits: List of expected bits (0 or 1).
            recovered_bits: List of received bits (0 or 1).

        Returns:
            Float representing the fraction of erroneous bits (0.0 to 1.0).
        """
        if not original_bits:
            return 0.0

        min_len = min(len(original_bits), len(recovered_bits))
        mismatches = sum(1 for i in range(min_len) if original_bits[i] != recovered_bits[i])

        # Account for length differences (e.g., dropped or excess bits)
        length_diff = abs(len(original_bits) - len(recovered_bits))
        total_errors = mismatches + length_diff

        return total_errors / max(len(original_bits), len(recovered_bits))
