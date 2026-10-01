"""
Smoke and Foundation Tests
==========================
Verifies that all project packages, placeholder modules, and initial utilities
are properly configured and importable.
"""

import sys
from pathlib import Path
import pytest

# Ensure app package is accessible in PYTHONPATH
project_root = Path(__file__).resolve().parent.parent
if str(project_root) not in sys.path:
    sys.path.insert(0, str(project_root))

try:
    from archive.legacy_python_backend.app.main import main
    from archive.legacy_python_backend.app.encoder.text_encoder import TextEncoder
    from archive.legacy_python_backend.app.encoder.error_control import ErrorControl
    from archive.legacy_python_backend.app.methods.sequence_method import SequenceSignalingMethod
    from archive.legacy_python_backend.app.methods.window_method import WindowSignalingMethod
    from archive.legacy_python_backend.app.methods.timing_method import TimingSignalingMethod
    from archive.legacy_python_backend.app.receiver.decoder import TextDecoder
    from archive.legacy_python_backend.app.receiver.reconstruction import StreamReconstructor
    from archive.legacy_python_backend.app.network.sender import SimulatedSender
    from archive.legacy_python_backend.app.network.receiver import SimulatedReceiver
    from archive.legacy_python_backend.app.network.packet_capture import PacketCaptureLogger
    from archive.legacy_python_backend.app.detection.timing_analysis import TimingAnalyzer
    from archive.legacy_python_backend.app.detection.header_analysis import HeaderAnalyzer
    from archive.legacy_python_backend.app.detection.anomaly_detector import SignalingAnomalyDetector
    from archive.legacy_python_backend.app.verification.hash_verify import IntegrityVerifier
except ImportError:
    from app.main import main
    from app.encoder.text_encoder import TextEncoder
    from app.encoder.error_control import ErrorControl
    from app.methods.sequence_method import SequenceSignalingMethod
    from app.methods.window_method import WindowSignalingMethod
    from app.methods.timing_method import TimingSignalingMethod
    from app.receiver.decoder import TextDecoder
    from app.receiver.reconstruction import StreamReconstructor
    from app.network.sender import SimulatedSender
    from app.network.receiver import SimulatedReceiver
    from app.network.packet_capture import PacketCaptureLogger
    from app.detection.timing_analysis import TimingAnalyzer
    from app.detection.header_analysis import HeaderAnalyzer
    from app.detection.anomaly_detector import SignalingAnomalyDetector
    from app.verification.hash_verify import IntegrityVerifier


def test_main_execution(capsys):
    """Verify that main() runs cleanly and demonstrates Step 2 binary encoding."""
    exit_code = main()
    assert exit_code == 0

    captured = capsys.readouterr()
    assert "TCP HEADER DATA MODULATION AND SIGNALING" in captured.out
    assert "STEP 2 — BINARY ENCODING" in captured.out
    assert "Round-trip verification:\nPASSED" in captured.out
    assert "CRC-16 ERROR DETECTION DEMONSTRATION" in captured.out
    assert "HAMMING(7,4) ERROR CORRECTION DEMONSTRATION" in captured.out


def test_classes_can_be_instantiated():
    """Verify that foundational classes instantiate without errors."""
    encoder = TextEncoder()
    assert encoder.encoding == "utf-8"

    seq_method = SequenceSignalingMethod(base_seq=5000)
    assert seq_method.base_seq == 5000

    win_method = WindowSignalingMethod(step_size=128)
    assert win_method.step_size == 128

    time_method = TimingSignalingMethod(delay_zero_ms=15.0, delay_one_ms=45.0)
    assert time_method.delay_zero_ms == 15.0
    assert time_method.delay_one_ms == 45.0

    decoder = TextDecoder()
    assert decoder.encoding == "utf-8"

    reconstructor = StreamReconstructor()
    assert reconstructor.buffered_symbols == []

    sender = SimulatedSender()
    assert sender.channel is None

    receiver = SimulatedReceiver()
    assert receiver.inbox == []

    logger = PacketCaptureLogger()
    assert logger.capture_dir == "captures"

    timing_analyzer = TimingAnalyzer()
    assert timing_analyzer is not None

    header_analyzer = HeaderAnalyzer()
    assert header_analyzer is not None

    detector = SignalingAnomalyDetector(confidence_threshold=0.8)
    assert detector.confidence_threshold == 0.8


def test_integrity_verifier():
    """Verify SHA-256 computation and Bit Error Rate (BER) calculations."""
    msg = "TCP Signaling Test"
    hash_val = IntegrityVerifier.compute_sha256(msg)
    # Expected SHA-256 is 64 hex characters
    assert len(hash_val) == 64
    assert isinstance(hash_val, str)

    # Exact match test
    assert IntegrityVerifier.verify_exact_match(msg, "TCP Signaling Test") is True
    assert IntegrityVerifier.verify_exact_match(msg, "Different Message") is False

    # Bit error rate test: identical bits -> BER = 0.0
    bits_a = [1, 0, 1, 1, 0]
    bits_b = [1, 0, 1, 1, 0]
    assert IntegrityVerifier.calculate_bit_error_rate(bits_a, bits_b) == 0.0

    # 1 error out of 5 bits -> BER = 0.2
    bits_c = [1, 0, 0, 1, 0]
    assert pytest.approx(IntegrityVerifier.calculate_bit_error_rate(bits_a, bits_c), 0.01) == 0.2


def test_directory_structure():
    """Verify that all required directories exist in the project root."""
    required_dirs = [
        project_root / "backend",
        project_root / "backend" / "include",
        project_root / "backend" / "src",
        project_root / "backend" / "tests",
        project_root / "frontend",
        project_root / "frontend" / "templates",
        project_root / "frontend" / "static",
        project_root / "archive" / "legacy_python_backend",
        project_root / "tests",
        project_root / "captures",
        project_root / "results",
    ]

    for directory in required_dirs:
        assert directory.exists(), f"Required directory missing: {directory}"
        assert directory.is_dir(), f"Path is not a directory: {directory}"
