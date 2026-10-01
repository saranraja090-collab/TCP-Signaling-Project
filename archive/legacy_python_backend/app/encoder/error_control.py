"""
Error Control Module
====================
Provides error detection (CRC) and error correction (Hamming(7,4))
mechanisms for binary signaling bitstreams.

Educational Networking Concepts:
--------------------------------
1. Error Detection vs. Error Correction:
   - Detection (e.g., CRC): Informs the receiver that one or more bits were corrupted.
     The receiver knows the frame is invalid, but cannot fix it.
   - Correction (e.g., Hamming): Adds enough structured redundancy that the receiver
     can both locate and invert flipped bits without requesting retransmission.

2. Important Limits:
   - CRC DETECTS errors; it DOES NOT correct them.
   - CRC does NOT guarantee error-free transmission under extreme noise.
   - Hamming(7,4) corrects up to 1 single-bit error per 7-bit codeword.
   - Neither mechanism recovers from packet drops or packet reordering;
     packet loss is handled at the network/transport layer.
"""

from typing import Tuple, Union


class CRC:
    """Cyclic Redundancy Check (CRC) error detection using Modulo-2 binary division.

    Standard Polynomial used: CRC-16-CCITT (degree 16)
    Polynomial: x^16 + x^12 + x^5 + 1
    Binary representation: '10001000000100001' (17 bits)
    """

    # CRC-16-CCITT generator polynomial (17 bits for degree 16)
    DEFAULT_POLYNOMIAL = "10001000000100001"

    def __init__(self, polynomial: str = DEFAULT_POLYNOMIAL):
        """Initialize CRC generator with a binary polynomial string.

        Args:
            polynomial: Binary string representing divisor polynomial (must start with '1').
        """
        if not polynomial or set(polynomial) - {"0", "1"} or polynomial[0] != "1":
            raise ValueError("Generator polynomial must be a non-empty binary string starting with '1'.")
        self.polynomial = polynomial
        self.degree = len(polynomial) - 1  # Number of CRC bits appended

    def _modulo2_division(self, dividend: str, divisor: str) -> str:
        """Perform modulo-2 binary long division (bitwise XOR division).

        Args:
            dividend: Binary string representing numerator.
            divisor: Binary string representing denominator.

        Returns:
            Remainder string of length len(divisor) - 1.
        """
        k = len(divisor)
        # Work with mutable list of characters for fast in-place XOR
        remainder = list(dividend[:k])

        for i in range(k, len(dividend) + 1):
            # If the leading bit is '1', XOR with the divisor
            if remainder[0] == "1":
                remainder = [
                    "0" if remainder[j] == divisor[j] else "1"
                    for j in range(k)
                ]
            # Discard the leading bit and bring down the next bit from dividend
            if i < len(dividend):
                remainder = remainder[1:] + [dividend[i]]
            else:
                remainder = remainder[1:]

        return "".join(remainder)

    def compute_crc(self, bitstream: str) -> str:
        """Generate CRC checksum bits for the given binary bitstream.

        Args:
            bitstream: String of '0' and '1' characters.

        Returns:
            A binary string of length self.degree representing the CRC checksum.
        """
        if not bitstream:
            return "0" * self.degree

        if set(bitstream) - {"0", "1"}:
            raise ValueError("Bitstream contains invalid non-binary characters.")

        # Step 1: Append degree zeros to the end of the message bitstream
        padded_stream = bitstream + ("0" * self.degree)

        # Step 2: Compute remainder using modulo-2 division
        return self._modulo2_division(padded_stream, self.polynomial)

    def append_crc(self, bitstream: str) -> str:
        """Append CRC checksum to the end of the bitstream for transmission.

        Args:
            bitstream: Input binary string.

        Returns:
            Transmitted bitstream: original bits + CRC bits.
        """
        crc_bits = self.compute_crc(bitstream)
        return bitstream + crc_bits

    def verify_crc(self, bitstream_with_crc: str) -> bool:
        """Verify whether the received bitstream with appended CRC is error-free.

        Args:
            bitstream_with_crc: Received binary string (data + CRC).

        Returns:
            True if remainder is all zeros (valid), False if corruption detected.
        """
        if not bitstream_with_crc or len(bitstream_with_crc) < self.degree:
            return False

        if set(bitstream_with_crc) - {"0", "1"}:
            return False

        remainder = self._modulo2_division(bitstream_with_crc, self.polynomial)
        return all(bit == "0" for bit in remainder)

    def extract_data(self, bitstream_with_crc: str) -> str:
        """Strip the CRC checksum from the received bitstream to recover the data bits.

        Args:
            bitstream_with_crc: Received binary string.

        Returns:
            Original data bits without CRC.
        """
        if len(bitstream_with_crc) < self.degree:
            raise ValueError("Bitstream is shorter than CRC checksum length.")
        return bitstream_with_crc[: -self.degree]


class Hamming74:
    """Hamming(7,4) Error-Correcting Code.

    Properties:
    - Encodes 4 data bits (d1, d2, d3, d4) into a 7-bit codeword (p1, p2, d1, p3, d2, d3, d4).
    - Capable of detecting and correcting any single-bit error within each 7-bit codeword.
    - Does NOT guarantee recovery against 2 or more bit flips within the same codeword.
    - Does NOT recover dropped packets (packet loss).
    """

    @staticmethod
    def encode_nibble(data_bits: list[int]) -> list[int]:
        """Encode 4 data bits into a 7-bit Hamming codeword.

        Bit positions (1-indexed):
        p1 = pos 1 (checks 1, 3, 5, 7)
        p2 = pos 2 (checks 2, 3, 6, 7)
        d1 = pos 3
        p3 = pos 4 (checks 4, 5, 6, 7)
        d2 = pos 5
        d3 = pos 6
        d4 = pos 7

        Args:
            data_bits: List of 4 binary integers [d1, d2, d3, d4].

        Returns:
            List of 7 binary integers [p1, p2, d1, p3, d2, d3, d4].
        """
        if len(data_bits) != 4 or not all(b in (0, 1) for b in data_bits):
            raise ValueError("Expected exactly 4 binary bits for Hamming(7,4) encoding.")

        d1, d2, d3, d4 = data_bits

        # Even parity equations (XOR)
        p1 = d1 ^ d2 ^ d4
        p2 = d1 ^ d3 ^ d4
        p3 = d2 ^ d3 ^ d4

        return [p1, p2, d1, p3, d2, d3, d4]

    @staticmethod
    def decode_nibble(codeword: list[int]) -> Tuple[list[int], bool, int]:
        """Decode a 7-bit Hamming codeword, correcting single-bit errors if present.

        Args:
            codeword: List of 7 received binary integers.

        Returns:
            Tuple of (corrected_data_bits [4], was_corrected (bool), error_position (1-indexed, 0 if clean))
        """
        if len(codeword) != 7 or not all(b in (0, 1) for b in codeword):
            raise ValueError("Expected exactly 7 binary bits for Hamming(7,4) decoding.")

        r = list(codeword)  # Copy to allow in-place correction

        # Syndrome calculation (even parity checks)
        # s1 checks positions 1, 3, 5, 7
        s1 = r[0] ^ r[2] ^ r[4] ^ r[6]
        # s2 checks positions 2, 3, 6, 7
        s2 = r[1] ^ r[2] ^ r[5] ^ r[6]
        # s3 checks positions 4, 5, 6, 7
        s3 = r[3] ^ r[4] ^ r[5] ^ r[6]

        # Syndrome value corresponds to the 1-indexed position of the erroneous bit
        error_pos = (s3 << 2) | (s2 << 1) | s1

        was_corrected = False
        if error_pos != 0:
            # Single-bit error detected at position error_pos (1-indexed)
            # Flip the corrupted bit back to its correct state
            r[error_pos - 1] ^= 1
            was_corrected = True

        # Extract data bits: positions 3, 5, 6, 7 (0-indexed: 2, 4, 5, 6)
        corrected_data = [r[2], r[4], r[5], r[6]]
        return corrected_data, was_corrected, error_pos

    @classmethod
    def encode_stream(cls, bitstream: str) -> Tuple[str, int]:
        """Encode an arbitrary binary bitstream using Hamming(7,4).

        Args:
            bitstream: String of '0' and '1' characters.

        Returns:
            Tuple of (encoded_bitstream_str, padding_bits_count).
        """
        if not bitstream:
            return "", 0

        # Pad bitstream with trailing zeros so length is a multiple of 4
        remainder = len(bitstream) % 4
        padding_count = (4 - remainder) if remainder != 0 else 0
        padded_stream = bitstream + ("0" * padding_count)

        encoded_chunks = []
        for i in range(0, len(padded_stream), 4):
            nibble = [int(b) for b in padded_stream[i : i + 4]]
            codeword = cls.encode_nibble(nibble)
            encoded_chunks.append("".join(str(b) for b in codeword))

        return "".join(encoded_chunks), padding_count

    @classmethod
    def decode_stream(cls, encoded_bitstream: str, padding_count: int = 0) -> Tuple[str, int]:
        """Decode a stream of 7-bit Hamming codewords and correct single-bit errors.

        Args:
            encoded_bitstream: Binary string of length multiple of 7.
            padding_count: Number of padding bits to strip from the tail.

        Returns:
            Tuple of (decoded_bitstream_str, total_corrected_errors).
        """
        if not encoded_bitstream:
            return "", 0

        if len(encoded_bitstream) % 7 != 0:
            raise ValueError("Encoded bitstream length must be a multiple of 7.")

        decoded_chunks = []
        total_corrected = 0

        for i in range(0, len(encoded_bitstream), 7):
            codeword = [int(b) for b in encoded_bitstream[i : i + 7]]
            data_bits, corrected, _ = cls.decode_nibble(codeword)
            if corrected:
                total_corrected += 1
            decoded_chunks.append("".join(str(b) for b in data_bits))

        full_decoded = "".join(decoded_chunks)
        if padding_count > 0:
            full_decoded = full_decoded[:-padding_count]

        return full_decoded, total_corrected


class ErrorControl:
    """Unified interface for error detection and correction tools."""

    @staticmethod
    def add_simple_parity(bits: list[int]) -> list[int]:
        """Appends an even parity bit to every byte (8 bits) of data.

        Args:
            bits: List of binary integer bits (length must be multiple of 8).

        Returns:
            List of bits with parity appended to each 8-bit block (9 bits per byte).
        """
        if len(bits) % 8 != 0:
            raise ValueError("Input bits length must be a multiple of 8.")

        result = []
        for i in range(0, len(bits), 8):
            byte = bits[i : i + 8]
            parity = sum(byte) % 2
            result.extend(byte)
            result.append(parity)
        return result
