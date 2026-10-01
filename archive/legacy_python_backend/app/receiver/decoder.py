"""
Decoder Module
==============
Translates recovered binary bit sequences back into ASCII / UTF-8 strings.
Acts as the receiver-side counterpart to the TextEncoder.
"""

from typing import Union
from app.encoder.text_encoder import TextEncoder


class TextDecoder:
    """Decodes binary bitstreams into text messages at the receiver."""

    def __init__(self, encoding: str = "utf-8"):
        """Initialize decoder with specified character encoding (default: UTF-8)."""
        self.encoding = encoding
        self._encoder = TextEncoder(encoding=encoding)

    def decode_bits(self, bitstream: Union[str, list[int]]) -> str:
        """Convert a binary bitstream (string or list of bits) back into text.

        Args:
            bitstream: String of '0' and '1' characters, or list of ints.

        Returns:
            The decoded plaintext string.
        """
        return self._encoder.decode_bits(bitstream)

    def bits_to_text(self, bits: list[int]) -> str:
        """Convert a list of binary bits (0 and 1) back into a text string.

        Args:
            bits: List of binary integer bits.

        Returns:
            Decoded text string.
        """
        return self._encoder.bits_to_text(bits)
