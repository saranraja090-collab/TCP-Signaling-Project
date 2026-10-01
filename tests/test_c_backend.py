"""
Unit and Integration Tests for C Networking Backend
===================================================
Executes the compiled C backend executable directly via subprocess
and validates:
1. Default Banner and JSON Status (Step 1 & Step 4 Scope).
2. Sequence-Number Signaling (Step 2).
3. Window-Size Signaling (Step 3).
4. Simulated Network Layer with Packet Loss, Corruption, and Reordering (Step 4).
5. Timing Signaling Removal (Permanently excluded from scope).
6. Strict 0-byte payload invariant across all conditions.
"""

import sys
import json
import subprocess
from pathlib import Path
import pytest

PROJECT_ROOT = Path(__file__).resolve().parent.parent
BACKEND_EXE = PROJECT_ROOT / "backend" / "bin" / "backend_engine.exe"
if not BACKEND_EXE.exists() and (PROJECT_ROOT / "backend" / "bin" / "backend.exe").exists():
    BACKEND_EXE = PROJECT_ROOT / "backend" / "bin" / "backend.exe"


@pytest.fixture(scope="session")
def check_c_backend_compiled():
    """Verify that backend executable exists before running tests."""
    assert BACKEND_EXE.exists(), (
        f"Backend executable not found at {BACKEND_EXE}. Run 'make' in backend/ first."
    )


def test_c_backend_banner(check_c_backend_compiled):
    """Test default execution produces the exact required banner with active methods."""
    proc = subprocess.run([str(BACKEND_EXE)], capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    out = proc.stdout

    assert "TCP HEADER DATA MODULATION AND SIGNALING" in out
    assert "C BACKEND" in out
    assert "Backend Status: READY" in out
    assert "1. Sequence Number" in out
    assert "2. Window Size" in out
    assert "3. Timing" not in out
    assert "In-Memory Simulation" in out
    assert "0 bytes" in out


def test_c_backend_json_status(check_c_backend_compiled):
    """Test --json / --status flag produces valid status JSON with only active methods."""
    proc = subprocess.run([str(BACKEND_EXE), "--json"], capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    assert data["backend"] == "C"
    assert data["status"] == "READY"
    assert data["payload_bytes"] == 0
    assert len(data["methods"]) == 2
    method_ids = [m["id"] for m in data["methods"]]
    assert "sequence" in method_ids
    assert "window" in method_ids
    assert "timing" not in method_ids


def test_c_backend_timing_rejected(check_c_backend_compiled):
    """Verify that timing signaling is rejected by backend engine."""
    cmd = [str(BACKEND_EXE), "-m", "timing", "-d", "TEST", "--json"]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode != 0
    assert "timing" in proc.stdout.lower() or "timing" in proc.stderr.lower()


def test_sequence_cli_text_output(check_c_backend_compiled):
    """Test CLI output format for sequence signaling per Step 2 specification."""
    msg = "HELLO NETWORK"
    cmd = [str(BACKEND_EXE), "-m", "sequence", "-d", msg]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)

    assert proc.returncode == 0
    out = proc.stdout

    assert "========================================" in out
    assert "SEQUENCE NUMBER SIGNALING" in out
    assert "Original Message:\nHELLO NETWORK" in out
    assert "Number of Bits:\n104" in out
    assert "Number of Packets:\n104" in out
    assert "Payload Per Packet:\n0 bytes" in out
    assert "Sequence Mapping:\n0 -> +100\n1 -> +200" in out
    assert "PACKET STREAM" in out
    assert "Packet 0:\nSEQ = 10100\nPayload = 0" in out
    assert "RECEIVER" in out
    assert "Decoded Message:\nHELLO NETWORK" in out
    assert "Integrity:\nVERIFIED" in out


def test_sequence_hello_network_json(check_c_backend_compiled):
    """Test Step 2 sequence JSON response keys and exact message recovery."""
    msg = "HELLO NETWORK"
    cmd = [str(BACKEND_EXE), "-m", "sequence", "-d", msg, "--json"]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)

    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    assert data["status"] == "success"
    assert data["method"] == "sequence"
    assert data["original_message"] == msg
    assert data["decoded_message"] == msg
    assert data["reconstructed_message"] == msg
    assert data["payload_bytes"] == 0
    assert data["packet_count"] == 104
    assert data["packets_count"] == 104
    assert data["bits_encoded"] == 104
    assert data["bit_error_rate"] == 0.0
    assert data["integrity"] == "verified"
    assert data["integrity_match"] is True

    # Sequence mapping verification
    assert data["sequence_mapping"]["bit_0"] == "+100"
    assert data["sequence_mapping"]["bit_1"] == "+200"
    assert data["sequence_mapping"]["base_seq"] == 10000

    # Ensure all sample packets have payload = 0
    for pkt in data["sample_packets"]:
        assert pkt["payload_b"] == 0


@pytest.mark.parametrize("method_name", ["sequence", "window"])
def test_c_backend_signaling_methods(check_c_backend_compiled, method_name):
    """Test active signaling methods execute cleanly with 0-byte payload invariant."""
    test_msg = "NETWORKS"
    cmd = [str(BACKEND_EXE), "-m", method_name, "-d", test_msg, "--json"]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)

    assert proc.returncode == 0, f"Backend failed with stderr: {proc.stderr}"
    data = json.loads(proc.stdout)

    assert data["status"] == "success"
    assert data["original_message"] == test_msg
    assert data["reconstructed_message"] == test_msg
    assert data["payload_bytes"] == 0
    assert data["bit_error_rate"] == 0.0
    assert data["integrity_match"] is True
    assert data["integrity"] == "verified"

    for pkt in data["sample_packets"]:
        assert pkt["payload_b"] == 0


def test_window_cli_text_output(check_c_backend_compiled):
    """Test CLI output format for Window Size signaling per Step 3 specification."""
    msg = "HELLO NETWORK"
    cmd = [str(BACKEND_EXE), "-m", "window", "-d", msg]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)

    assert proc.returncode == 0
    out = proc.stdout

    assert "========================================" in out
    assert "WINDOW SIZE SIGNALING" in out
    assert "Original Message:\nHELLO NETWORK" in out
    assert "Number of Bits:\n104" in out
    assert "Number of Packets:\n104" in out
    assert "Payload Per Packet:\n0 bytes" in out
    assert "Window Mapping:\n0 -> 30000\n1 -> 60000" in out
    assert "PACKET STREAM" in out
    assert "WINDOW = 30000" in out
    assert "WINDOW = 60000" in out
    assert "Payload = 0" in out
    assert "RECEIVER" in out
    assert "Decoded Message:\nHELLO NETWORK" in out
    assert "Integrity:\nVERIFIED" in out


def test_window_hello_network_json(check_c_backend_compiled):
    """Test Step 3 Window Size JSON output, window mapping, and 0-byte payload invariant."""
    msg = "HELLO NETWORK"
    cmd = [str(BACKEND_EXE), "-m", "window", "-d", msg, "--json"]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)

    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    assert data["status"] == "success"
    assert data["method"] == "window"
    assert data["method_name"] == "Window Size"
    assert data["execution_mode"] == "In-Memory Simulation"
    assert data["original_message"] == msg
    assert data["decoded_message"] == msg
    assert data["reconstructed_message"] == msg
    assert data["payload_bytes"] == 0
    assert data["packet_count"] == 104
    assert data["packets_count"] == 104
    assert data["bits_encoded"] == 104
    assert data["bit_error_rate"] == 0.0
    assert data["integrity"] == "verified"
    assert data["integrity_match"] is True

    # Window mapping verification
    assert data["window_mapping"]["bit_0"] == 30000
    assert data["window_mapping"]["bit_1"] == 60000

    # Ensure all sample packets have payload = 0 and valid window sizes
    for pkt in data["sample_packets"]:
        assert pkt["payload_b"] == 0
        assert pkt["win"] in (30000, 60000)


# ==============================================================================
# Step 4: Network Impairment & Simulation Tests
# ==============================================================================

def test_step4_network_packet_loss(check_c_backend_compiled):
    """Test packet loss simulation in C backend and telemetry tracking."""
    cmd = [
        str(BACKEND_EXE), "-m", "sequence", "-d", "TEST MESSAGE",
        "--loss", "20", "--seed", "12345", "--json"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    stats = data["packet_statistics"]
    assert stats["generated"] > 0
    assert stats["lost"] > 0
    assert stats["received"] == stats["generated"] - stats["lost"]
    assert data["payload_bytes"] == 0

    # Status must reflect loss / incomplete
    assert data["status"] in ("incomplete", "failed", "error")
    assert any("LOSS" in err or "SEQUENCE" in err for err in data["detected_errors"])


def test_step4_network_packet_corruption(check_c_backend_compiled):
    """Test packet corruption simulation in C backend."""
    cmd = [
        str(BACKEND_EXE), "-m", "sequence", "-d", "TEST MESSAGE",
        "--corrupt", "25", "--seed", "9999", "--json"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    stats = data["packet_statistics"]
    assert stats["corrupted"] > 0
    assert stats["lost"] == 0
    assert stats["received"] == stats["generated"]
    assert data["payload_bytes"] == 0

    # Receiver anomaly detection
    assert "INVALID_SEQUENCE_DELTA" in data["detected_errors"]


def test_step4_network_packet_reordering(check_c_backend_compiled):
    """Test packet reordering simulation in C backend."""
    cmd = [
        str(BACKEND_EXE), "-m", "sequence", "-d", "TEST MESSAGE",
        "--reorder", "30", "--seed", "42", "--json"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    stats = data["packet_statistics"]
    assert stats["reordered"] > 0
    assert stats["lost"] == 0
    assert data["payload_bytes"] == 0

    # Receiver anomaly detection
    assert "PACKET_ORDER_ERROR" in data["detected_errors"]


def test_step4_deterministic_seed(check_c_backend_compiled):
    """Test that specifying the same random seed produces identical results."""
    cmd1 = [
        str(BACKEND_EXE), "-m", "sequence", "-d", "DETERMINISTIC TEST",
        "--loss", "15", "--corrupt", "10", "--reorder", "20", "--seed", "777", "--json"
    ]
    proc1 = subprocess.run(cmd1, capture_output=True, text=True, timeout=5)
    assert proc1.returncode == 0
    data1 = json.loads(proc1.stdout)

    proc2 = subprocess.run(cmd1, capture_output=True, text=True, timeout=5)
    assert proc2.returncode == 0
    data2 = json.loads(proc2.stdout)

    assert data1["packet_statistics"] == data2["packet_statistics"]
    assert data1["detected_errors"] == data2["detected_errors"]


def test_step4_payload_remains_strictly_zero_under_all_impairments(check_c_backend_compiled):
    """Verify that every packet in flow and telemetry has payload = 0 under full impairment."""
    cmd = [
        str(BACKEND_EXE), "-m", "window", "-d", "ZERO PAYLOAD INVARIANT",
        "--loss", "10", "--corrupt", "10", "--reorder", "10", "--seed", "555", "--json"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    assert data["payload_bytes"] == 0

    for pkt in data.get("sample_packets", []):
        assert pkt["payload_b"] == 0

    for pkt in data.get("packet_flow", []):
        assert pkt["payload_b"] == 0


# ==============================================================================
# Step 5: Comparative Analysis Tests
# ==============================================================================

def test_step5_compare_cli_output(check_c_backend_compiled):
    """Test CLI output formatting for Step 5 comparison mode."""
    cmd = [str(BACKEND_EXE), "-m", "compare", "-d", "HELLO NETWORK"]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    out = proc.stdout

    assert "SIGNALING METHOD COMPARISON" in out
    assert "Original Message:\nHELLO NETWORK" in out
    assert "SEQUENCE NUMBER" in out
    assert "WINDOW SIZE" in out
    assert "Packets Generated : 104" in out
    assert "Payload           : 0 bytes" in out
    assert "Decoded Message:\nHELLO NETWORK" in out
    assert "Integrity:\nVERIFIED" in out


def test_step5_compare_json_baseline(check_c_backend_compiled):
    """Test baseline comparison (loss=0, corrupt=0, reorder=0) produces verified results for both."""
    cmd = [str(BACKEND_EXE), "-m", "compare", "-d", "HELLO NETWORK", "--json"]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    assert data["status"] == "success"
    assert data["mode"] == "comparison"
    assert data["payload_bytes"] == 0
    assert data["experiment"]["message"] == "HELLO NETWORK"

    # Both methods generated 104 packets and 0 lost
    seq = data["sequence"]
    win = data["window"]

    assert seq["method"] == "sequence"
    assert seq["packets_generated"] == 104
    assert seq["packets_received"] == 104
    assert seq["packets_lost"] == 0
    assert seq["decoded_message"] == "HELLO NETWORK"
    assert seq["integrity_match"] is True
    assert seq["integrity"] == "verified"

    assert win["method"] == "window"
    assert win["packets_generated"] == 104
    assert win["packets_received"] == 104
    assert win["packets_lost"] == 0
    assert win["decoded_message"] == "HELLO NETWORK"
    assert win["integrity_match"] is True
    assert win["integrity"] == "verified"


def test_step5_compare_packet_loss(check_c_backend_compiled):
    """Test comparative behavior under 10% packet loss."""
    cmd = [
        str(BACKEND_EXE), "-m", "compare", "-d", "NETWORK LOSS",
        "--loss", "15", "--seed", "4242", "--json"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    seq = data["sequence"]
    win = data["window"]

    # Both must encounter the identical loss under the same seed
    assert seq["packets_lost"] > 0
    assert win["packets_lost"] > 0
    assert seq["packets_lost"] == win["packets_lost"]
    assert seq["packets_received"] == win["packets_received"]
    assert data["payload_bytes"] == 0


def test_step5_compare_header_corruption(check_c_backend_compiled):
    """Test comparative behavior under 20% header corruption."""
    cmd = [
        str(BACKEND_EXE), "-m", "compare", "-d", "CORRUPTION TEST",
        "--corrupt", "20", "--seed", "12345", "--json"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    seq = data["sequence"]
    win = data["window"]

    assert seq["packets_corrupted"] > 0
    assert win["packets_corrupted"] > 0
    assert seq["packets_corrupted"] == win["packets_corrupted"]
    assert "INVALID_SEQUENCE_DELTA" in seq["detected_errors"]
    assert "INVALID_WINDOW_VALUE" in win["detected_errors"]


def test_step5_compare_packet_reordering(check_c_backend_compiled):
    """Test comparative behavior under 25% packet reordering."""
    cmd = [
        str(BACKEND_EXE), "-m", "compare", "-d", "REORDERING TEST",
        "--reorder", "25", "--seed", "9999", "--json"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    seq = data["sequence"]
    win = data["window"]

    assert seq["packets_reordered"] > 0
    assert win["packets_reordered"] > 0
    assert seq["packets_reordered"] == win["packets_reordered"]
    assert "PACKET_ORDER_ERROR" in seq["detected_errors"]


def test_step5_compare_combined_impairments(check_c_backend_compiled):
    """Test comparative behavior under combined loss, corruption, and reordering."""
    cmd = [
        str(BACKEND_EXE), "-m", "compare", "-d", "COMBINED COMPARISON",
        "--loss", "10", "--corrupt", "10", "--reorder", "15", "--seed", "777", "--json"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    assert data["payload_bytes"] == 0
    assert data["sequence"]["packets_generated"] == data["window"]["packets_generated"]
    assert data["sequence"]["packets_lost"] == data["window"]["packets_lost"]
    assert data["sequence"]["packets_corrupted"] == data["window"]["packets_corrupted"]
    assert data["sequence"]["packets_reordered"] == data["window"]["packets_reordered"]


def test_step5_compare_payload_strictly_zero(check_c_backend_compiled):
    """Verify strict 0-byte payload invariant across all comparison modes."""
    cmd = [
        str(BACKEND_EXE), "-m", "compare", "-d", "ZERO BYTE PAYLOAD CHECK",
        "--loss", "5", "--corrupt", "5", "--reorder", "5", "--seed", "111", "--json"
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
    assert proc.returncode == 0
    data = json.loads(proc.stdout)

    assert data["payload_bytes"] == 0

