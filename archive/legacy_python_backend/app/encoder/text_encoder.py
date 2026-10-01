"""
Text Encoder Module
===================
Converts human-readable text into binary bitstreams and decodes bitstreams
back into text messages using UTF-8 encoding.

Internal Representation Choice:
-------------------------------
We represent the bitstream as a Python string of '0' and '1' characters
(e.g., "0100100001000101").

Why this representation?
1. Beginner-Friendly: Highly visual, directly printable, and easy to inspect.
2. Modular & Slicable: Easy to partition into 8-bit bytes, 4-bit nibbles (Hamming),
   or 1-bit / 2-bit symbols for subsequent TCP signaling modulation.
3. Clean Validation: Simple to verify using string character sets.
4. Flexible Conversion: Easily converted to/from list of integers ([0, 1, ...])
   or raw bytes via helper methods.
"""

from typing import Union


class TextEncoder:
    """Encodes text into binary bitstreams and decodes bitstreams back to text."""

    def __init__(self, encoding: str = "utf-8"):
        """Initialize the encoder with the specified character encoding (default: UTF-8).

        Args:
            encoding: Text encoding standard (default 'utf-8').
        """
        self.encoding = encoding

    def encode_text(self, text: str) -> str:
        """Convert a text string into an 8-bit-per-byte binary bitstream string.

        Steps:
        1. Validate input type (must be string).
        2. Convert the string to UTF-8 bytes.
        3. Convert each byte (0-255) into an exact 8-character binary string (e.g. 72 -> '01001000').
        4. Concatenate all 8-bit binary strings into a single bitstream.

        Args:
            text: Input string (supports ASCII and multi-byte Unicode characters).

        Returns:
            A string containing only '0' and '1' characters. Empty string if input is empty.

        Raises:
            TypeError: If input is not a string.
        """
        if not isinstance(text, str):
            raise TypeError(f"Expected str input, got {type(text).__name__}")

        if not text:
            return ""

        # Step 1: Convert string to UTF-8 byte sequence
        raw_bytes = text.encode(self.encoding)

        # Step 2: Convert each byte into exactly 8 binary digits with leading zeros
        # Format specifier '08b': 0-padded, 8 digits, binary representation
        bit_chunks = [f"{byte:08b}" for byte in raw_bytes]

        # Step 3: Combine into a single continuous bitstream string
        return "".join(bit_chunks)

    def decode_bits(self, bitstream: Union[str, list[int]]) -> str:
        """Convert a binary bitstream back into the original text string.

        Steps:
        1. Validate input format and characters.
        2. Verify that total bit length is a multiple of 8 (each byte requires 8 bits).
        3. Slice bitstream into 8-bit chunks.
        4. Convert each 8-bit chunk from base-2 binary to an integer byte (0-255).
        5. Decode bytearray using UTF-8 back to original characters.

        Args:
            bitstream: String of '0' and '1' characters, or list of integers (0 and 1).

        Returns:
            The decoded plaintext string.

        Raises:
            TypeError: If bitstream is neither a string nor a list of ints.
            ValueError: If bitstream contains non-binary characters or length is not a multiple of 8.
            UnicodeDecodeError: If binary data does not represent valid UTF-8 sequences.
        """
        # Allow passing a list of integers [0, 1, 0, ...] for flexibility
        if isinstance(bitstream, list):
            # Validate list elements are 0 or 1
            if not all(isinstance(b, int) and b in (0, 1) for b in bitstream):
                raise ValueError("Bit list must contain only integers 0 or 1.")
            bitstream = "".join(str(b) for b in bitstream)

        if not isinstance(bitstream, str):
            raise TypeError(f"Expected str or list of ints for bitstream, got {type(bitstream).__name__}")

        # Empty bitstream decodes to empty string
        if not bitstream:
            return ""

        # Validate that all characters are '0' or '1'
        non_binary_chars = set(bitstream) - {"0", "1"}
        if non_binary_chars:
            raise ValueError(
                f"Invalid bitstream: contains non-binary characters {sorted(list(non_binary_chars))}"
            )

        # Check byte boundary: every byte consists of exactly 8 bits
        if len(bitstream) % 8 != 0:
            raise ValueError(
                f"Invalid bitstream length ({len(bitstream)}): must be a multiple of 8 bits (1 byte = 8 bits)."
            )

        # Slice into 8-bit bytes and convert to bytearray
        byte_list = bytearray()
        for i in range(0, len(bitstream), 8):
            byte_chunk = bitstream[i : i + 8]
            byte_value = int(byte_chunk, 2)
            byte_list.append(byte_value)

        # Decode UTF-8 bytes to string
        return byte_list.decode(self.encoding)

    def text_to_bits(self, text: str) -> list[int]:
        """Helper: Convert text directly into a list of individual integer bits (0 or 1).

        Args:
            text: Plaintext string.

        Returns:
            List of integers, each either 0 or 1.
        """
        bitstream_str = self.encode_text(text)
        return [int(b) for b in bitstream_str]

    def bits_to_text(self, bits: list[int]) -> str:
        """Helper: Convert a list of individual integer bits back into text.

        Args:
            bits: List of integers (0 or 1).

        Returns:
            Decoded plaintext string.
        """
        return self.decode_bits(bits)
