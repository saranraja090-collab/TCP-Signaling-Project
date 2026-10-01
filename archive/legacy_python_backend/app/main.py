"""
Main Entry Point
================
TCP Header Data Modulation and Signaling
STEP 2 — BINARY ENCODING AND ERROR CONTROL DEMONSTRATION

Demonstrates:
1. Converting text (ASCII and Unicode) into UTF-8 bytes and binary bitstreams.
2. Reconstructing original text from bitstreams (round-trip verification).
3. CRC-16 error detection in action (clean vs. corrupted bitstream).
4. Hamming(7,4) single-bit error detection and automatic correction.
"""

import sys
from pathlib import Path

# Configure UTF-8 for console output on Windows to support Unicode and emojis
if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")

# Ensure project root is in sys.path when running script directly
project_root = Path(__file__).resolve().parent.parent
if str(project_root) not in sys.path:
    sys.path.insert(0, str(project_root))

from app.encoder.text_encoder import TextEncoder
from app.receiver.decoder import TextDecoder
from app.encoder.error_control import CRC, Hamming74
from app.verification.hash_verify import IntegrityVerifier


def demonstrate_text_encoding(sample_text: str) -> bool:
    """Demonstrate encoding and decoding for a given text sample."""
    encoder = TextEncoder()
    decoder = TextDecoder()

    print("-" * 60)
    print(f"Original Message:\n{sample_text}")
    print()

    # Step 1: UTF-8 Bytes
    utf8_bytes = sample_text.encode("utf-8")
    hex_representation = " ".join(f"0x{b:02X}" for b in utf8_bytes)
    print(f"UTF-8 Bytes ({len(utf8_bytes)} bytes):\n{hex_representation}")
    print()

    # Step 2: Binary Bitstream
    bitstream = encoder.encode_text(sample_text)
    # Format bitstream into 8-bit groups for readability
    grouped_bits = " ".join(bitstream[i : i + 8] for i in range(0, len(bitstream), 8))
    print(f"Bitstream ({len(bitstream)} bits):\n{grouped_bits}")
    print()

    print(f"Number of bytes: {len(utf8_bytes)}")
    print(f"Number of bits : {len(bitstream)}")
    print()

    # Step 3: Decode back to plaintext
    decoded_message = decoder.decode_bits(bitstream)
    print(f"Decoded Message:\n{decoded_message}")
    print()

    # Verification
    is_match = IntegrityVerifier.verify_exact_match(sample_text, decoded_message)
    status_str = "PASSED" if is_match else "FAILED"
    print(f"Round-trip verification:\n{status_str}")
    print("-" * 60)
    return is_match


def demonstrate_crc_error_detection() -> None:
    """Demonstrate how CRC detects bit corruption."""
    print()
    print("=" * 60)
    print("CRC-16 ERROR DETECTION DEMONSTRATION")
    print("=" * 60)

    encoder = TextEncoder()
    crc = CRC()

    test_msg = "NET"
    raw_bits = encoder.encode_text(test_msg)
    transmitted = crc.append_crc(raw_bits)

    print(f"Message             : {test_msg}")
    print(f"Data Bits (24)      : {raw_bits}")
    print(f"CRC-16 Checksum (16): {transmitted[24:]}")
    print(f"Total Frame (40)    : {transmitted}")
    print(f"Receiver Check (Untouched)  : {'VALID (No error detected)' if crc.verify_crc(transmitted) else 'INVALID'}")

    # Introduce an intentional bit error (flip bit at index 5)
    corrupted_list = list(transmitted)
    original_bit = corrupted_list[5]
    corrupted_list[5] = "1" if original_bit == "0" else "0"
    corrupted = "".join(corrupted_list)

    print(f"\n[!] Simulating Noise: Flipped bit #5 from '{original_bit}' to '{corrupted_list[5]}'")
    print(f"Corrupted Frame     : {corrupted}")
    check_result = crc.verify_crc(corrupted)
    print(f"Receiver Check (Corrupted)  : {'VALID' if check_result else 'CORRUPTED (Error detected by CRC!)'}")
    print("Note: CRC reliably detects corruption, but cannot correct it.")


def demonstrate_hamming_error_correction() -> None:
    """Demonstrate how Hamming(7,4) detects and corrects single-bit errors."""
    print()
    print("=" * 60)
    print("HAMMING(7,4) ERROR CORRECTION DEMONSTRATION")
    print("=" * 60)

    # 4 data bits: [1, 0, 1, 1]
    data_bits = [1, 0, 1, 1]
    codeword = Hamming74.encode_nibble(data_bits)
    print(f"Original 4 Data Bits (d1, d2, d3, d4) : {data_bits}")
    print(f"Encoded 7-Bit Codeword (with 3 parity): {codeword}")

    # Simulate 1 bit flip at position 5 (pos 5 is d2)
    corrupted_codeword = list(codeword)
    flip_pos = 5
    corrupted_codeword[flip_pos - 1] ^= 1
    print(f"\n[!] Simulating Noise: Flipped bit at position {flip_pos} -> {corrupted_codeword}")

    # Receiver decodes and corrects
    recovered_bits, was_corrected, detected_pos = Hamming74.decode_nibble(corrupted_codeword)
    print(f"Syndrome Detected Error Position      : Position {detected_pos}")
    print(f"Automatic Error Correction Triggered  : {was_corrected}")
    print(f"Recovered 4 Data Bits                 : {recovered_bits}")
    print(f"Correction Result                     : {'SUCCESS' if recovered_bits == data_bits else 'FAILED'}")
    print("Note: Hamming(7,4) repairs single-bit errors without retransmission.")


def main() -> int:
    """Run Step 2 demonstration."""
    print("==================================================")
    print("TCP HEADER DATA MODULATION AND SIGNALING")
    print("STEP 2 — BINARY ENCODING AND ERROR CONTROL")
    print("==================================================")
    print()

    # Demonstration 1: Standard ASCII message
    demonstrate_text_encoding("HELLO NETWORK")

    # Demonstration 2: Unicode and Emojis
    demonstrate_text_encoding("TCP 🌐 Signaling")

    # Demonstration 3: CRC Error Detection
    demonstrate_crc_error_detection()

    # Demonstration 4: Hamming(7,4) Error Correction
    demonstrate_hamming_error_correction()

    print()
    print("=" * 60)
    print("Step 2 completed successfully. Ready for Step 3.")
    print("==================================================")
    return 0


if __name__ == "__main__":
    sys.exit(main())
