"""
Unit Tests for Step 2: Binary Text Encoding, Decoding, and Error Control
======================================================================
Tests verify that:
1. Strings (ASCII and Unicode) encode to exact 8-bit-per-byte binary representations.
2. Binary bitstreams decode back to exact original strings (round-trip fidelity).
3. Input validation correctly catches non-binary inputs, non-multiple lengths, and type errors.
4. CRC error detection successfully validates clean data and flags corrupted bits.
5. Hamming(7,4) code successfully detects and corrects single-bit errors.
"""

import pytest
try:
    from archive.legacy_python_backend.app.encoder.text_encoder import TextEncoder
    from archive.legacy_python_backend.app.receiver.decoder import TextDecoder
    from archive.legacy_python_backend.app.encoder.error_control import CRC, Hamming74, ErrorControl
except ImportError:
    from app.encoder.text_encoder import TextEncoder
    from app.receiver.decoder import TextDecoder
    from app.encoder.error_control import CRC, Hamming74, ErrorControl


@pytest.fixture
def encoder():
    """Fixture providing a fresh TextEncoder instance."""
    return TextEncoder()


@pytest.fixture
def decoder():
    """Fixture providing a fresh TextDecoder instance."""
    return TextDecoder()


# ---------------------------------------------------------------------------
# 1. Basic & Specific Word Encoding Tests
# ---------------------------------------------------------------------------

def test_encode_decode_hello(encoder, decoder):
    """Test encoding and decoding of 'HELLO'."""
    text = "HELLO"
    bitstream = encoder.encode_text(text)

    # 'H' = 0x48 = 01001000
    # 'E' = 0x45 = 01000101
    # 'L' = 0x4C = 01001100
    # 'L' = 0x4C = 01001100
    # 'O' = 0x4F = 01001111
    expected_bits = "0100100001000101010011000100110001001111"
    assert bitstream == expected_bits
    assert len(bitstream) == 5 * 8  # 40 bits

    # Reconstruct text using both encoder and receiver decoder
    decoded_text = encoder.decode_bits(bitstream)
    assert decoded_text == text
    assert decoder.decode_bits(bitstream) == text


def test_encode_decode_empty_string(encoder, decoder):
    """Test encoding and decoding of an empty string."""
    empty = ""
    bitstream = encoder.encode_text(empty)
    assert bitstream == ""
    assert len(bitstream) == 0

    assert encoder.decode_bits(bitstream) == ""
    assert decoder.decode_bits(bitstream) == ""


def test_encode_decode_computer_networks(encoder, decoder):
    """Test encoding and decoding of 'Computer Networks'."""
    text = "Computer Networks"
    bitstream = encoder.encode_text(text)

    # 17 characters in ASCII = 17 bytes = 136 bits
    assert len(bitstream) == len(text) * 8
    assert encoder.decode_bits(bitstream) == text
    assert decoder.decode_bits(bitstream) == text


def test_encode_decode_numbers_and_punctuation(encoder, decoder):
    """Test strings containing numbers, spaces, and punctuation symbols."""
    text = "TCP/IP v4 & v6: Port 80, 443! ($100 = 100%)"
    bitstream = encoder.encode_text(text)
    decoded = encoder.decode_bits(bitstream)
    assert decoded == text


def test_encode_decode_unicode_text(encoder, decoder):
    """Test multi-byte Unicode strings including emojis and non-Latin scripts."""
    test_cases = [
        "TCP Project",
        "தமிழ்",                 # Tamil script (multi-byte UTF-8)
        "Hello 🌐",              # Emoji globe (4-byte UTF-8)
        "ネットワーク (Network)", # Japanese Katakana
        "مرحبا بالعالم",         # Arabic text
    ]

    for sample in test_cases:
        bitstream = encoder.encode_text(sample)
        # Verify length in bits matches UTF-8 byte count * 8
        expected_bytes_count = len(sample.encode("utf-8"))
        assert len(bitstream) == expected_bytes_count * 8

        # Round-trip verification
        decoded = encoder.decode_bits(bitstream)
        assert decoded == sample, f"Failed roundtrip for Unicode sample: {sample}"


# ---------------------------------------------------------------------------
# 2. General Round-Trip & Invariance
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("message", [
    "A",
    "Short",
    "A somewhat longer sentence to thoroughly test byte chunking and decoding.",
    "Special characters: \n\t\r\\\"'<>?",
    "Binary signaling simulation 2026",
])
def test_round_trip_invariance(encoder, decoder, message):
    """For every valid test message: original == decode(encode(original))."""
    encoded = encoder.encode_text(message)
    decoded_from_encoder = encoder.decode_bits(encoded)
    decoded_from_receiver = decoder.decode_bits(encoded)

    assert decoded_from_encoder == message
    assert decoded_from_receiver == message


def test_helper_bit_list_methods(encoder, decoder):
    """Verify helper methods text_to_bits and bits_to_text with integer lists."""
    text = "OK"
    bits = encoder.text_to_bits(text)
    assert isinstance(bits, list)
    assert all(b in (0, 1) for b in bits)
    assert len(bits) == 16

    recovered = encoder.bits_to_text(bits)
    assert recovered == text

    # Receiver decoder also accepts list of bits
    assert decoder.bits_to_text(bits) == text


# ---------------------------------------------------------------------------
# 3. Input Validation and Error Handling
# ---------------------------------------------------------------------------

def test_invalid_text_type(encoder):
    """Verify TypeError is raised if non-string is passed to encode_text."""
    with pytest.raises(TypeError):
        encoder.encode_text(12345)  # type: ignore

    with pytest.raises(TypeError):
        encoder.encode_text(["list", "of", "strings"])  # type: ignore


def test_invalid_bitstream_characters(encoder):
    """Verify ValueError is raised if bitstream contains non-binary characters."""
    with pytest.raises(ValueError, match="non-binary"):
        encoder.decode_bits("01001002")

    with pytest.raises(ValueError, match="non-binary"):
        encoder.decode_bits("01001A00")

    with pytest.raises(ValueError, match="only integers 0 or 1"):
        encoder.decode_bits([0, 1, 2, 0])  # type: ignore


def test_invalid_bitstream_length_not_divisible_by_eight(encoder):
    """Verify ValueError when bitstream length is not a multiple of 8."""
    # 7 bits (1 short of 1 byte)
    with pytest.raises(ValueError, match="multiple of 8"):
        encoder.decode_bits("0100100")

    # 13 bits (not divisible by 8)
    with pytest.raises(ValueError, match="multiple of 8"):
        encoder.decode_bits("0100100011111")


# ---------------------------------------------------------------------------
# 4. CRC Error Detection Tests
# ---------------------------------------------------------------------------

def test_crc_generation_and_verification():
    """Verify CRC checksum generation and validation for clean bitstreams."""
    crc = CRC()
    bitstream = "0100100001000101"  # "HE"

    # Compute CRC bits (16 bits for CRC-16)
    crc_bits = crc.compute_crc(bitstream)
    assert len(crc_bits) == crc.degree
    assert set(crc_bits).issubset({"0", "1"})

    # Append CRC to bitstream
    transmitted = crc.append_crc(bitstream)
    assert len(transmitted) == len(bitstream) + crc.degree
    assert transmitted.startswith(bitstream)

    # Receiver verification of untouched bitstream
    assert crc.verify_crc(transmitted) is True

    # Extract original data
    recovered_data = crc.extract_data(transmitted)
    assert recovered_data == bitstream


def test_crc_detects_single_and_burst_bit_flips():
    """Verify that CRC detects corrupted bits."""
    crc = CRC()
    bitstream = "010010000100010101001100"  # "HEL"
    transmitted = crc.append_crc(bitstream)

    # Case 1: Single bit flip in message payload
    corrupted_list = list(transmitted)
    corrupted_list[3] = "1" if corrupted_list[3] == "0" else "0"  # flip 4th bit
    corrupted_stream = "".join(corrupted_list)
    assert crc.verify_crc(corrupted_stream) is False

    # Case 2: Bit flip in the CRC checksum itself
    corrupted_list = list(transmitted)
    corrupted_list[-1] = "1" if corrupted_list[-1] == "0" else "0"  # flip last CRC bit
    corrupted_stream = "".join(corrupted_list)
    assert crc.verify_crc(corrupted_stream) is False

    # Case 3: Burst error (multiple adjacent bits flipped)
    corrupted_list = list(transmitted)
    for idx in [5, 6, 7]:
        corrupted_list[idx] = "1" if corrupted_list[idx] == "0" else "0"
    corrupted_stream = "".join(corrupted_list)
    assert crc.verify_crc(corrupted_stream) is False


def test_crc_empty_and_invalid():
    """Verify CRC handling of empty and invalid inputs."""
    crc = CRC()
    assert crc.compute_crc("") == "0" * crc.degree
    assert crc.verify_crc("") is False
    assert crc.verify_crc("010") is False  # Shorter than degree

    with pytest.raises(ValueError):
        crc.compute_crc("010201")


# ---------------------------------------------------------------------------
# 5. Hamming(7,4) Error Correction Tests
# ---------------------------------------------------------------------------

def test_hamming74_clean_nibble():
    """Verify clean 4-bit nibble encoding and decoding without errors."""
    data = [1, 0, 1, 1]
    codeword = Hamming74.encode_nibble(data)
    assert len(codeword) == 7

    decoded_data, corrected, err_pos = Hamming74.decode_nibble(codeword)
    assert decoded_data == data
    assert corrected is False
    assert err_pos == 0


def test_hamming74_corrects_single_bit_flip_in_all_positions():
    """Verify that Hamming(7,4) detects and corrects a bit flip in any position (1 to 7)."""
    data = [1, 1, 0, 1]
    original_codeword = Hamming74.encode_nibble(data)

    # Test flipping every position from 1 to 7
    for pos_to_flip in range(1, 8):
        corrupted_codeword = list(original_codeword)
        # Invert bit at pos_to_flip (1-indexed -> index pos_to_flip - 1)
        corrupted_codeword[pos_to_flip - 1] ^= 1

        decoded_data, corrected, detected_pos = Hamming74.decode_nibble(corrupted_codeword)
        assert corrected is True
        assert detected_pos == pos_to_flip
        assert decoded_data == data, f"Failed to correct bit flipped at position {pos_to_flip}"


def test_hamming74_stream_encode_decode_with_errors():
    """Verify full stream encoding with Hamming(7,4) and recovery from bit errors."""
    encoder = TextEncoder()
    message = "NET"
    raw_bits = encoder.encode_text(message)

    # Encode bitstream into Hamming codewords
    encoded_stream, padding = Hamming74.encode_stream(raw_bits)
    assert len(encoded_stream) % 7 == 0

    # Inject 1 bit error into the first codeword (bits 0-6)
    corrupted_stream_list = list(encoded_stream)
    corrupted_stream_list[2] = "1" if corrupted_stream_list[2] == "0" else "0"
    # Inject 1 bit error into the third codeword (bits 14-20)
    corrupted_stream_list[18] = "1" if corrupted_stream_list[18] == "0" else "0"
    corrupted_stream = "".join(corrupted_stream_list)

    # Decode and correct errors
    recovered_bits, total_corrected = Hamming74.decode_stream(corrupted_stream, padding)
    assert total_corrected == 2
    assert recovered_bits == raw_bits

    # Verify message is completely restored
    recovered_text = encoder.decode_bits(recovered_bits)
    assert recovered_text == message


def test_error_control_simple_parity():
    """Verify parity bit appending function."""
    bits = [1, 0, 1, 0, 1, 0, 1, 0]  # sum is 4 (even) -> parity bit should be 0
    with_parity = ErrorControl.add_simple_parity(bits)
    assert len(with_parity) == 9
    assert with_parity == [1, 0, 1, 0, 1, 0, 1, 0, 0]

    bits_odd = [1, 0, 1, 0, 1, 0, 1, 1]  # sum is 5 (odd) -> parity bit should be 1
    with_parity_odd = ErrorControl.add_simple_parity(bits_odd)
    assert with_parity_odd == [1, 0, 1, 0, 1, 0, 1, 1, 1]
