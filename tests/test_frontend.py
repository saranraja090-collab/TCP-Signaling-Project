"""
Unit and Integration Tests for Python Flask Web Frontend
=========================================================
Tests the HTTP endpoints, HTML template rendering, C subprocess integration,
and Step 4 Network Impairment controls & telemetry.
"""

import sys
import json
from pathlib import Path
import pytest

PROJECT_ROOT = Path(__file__).resolve().parent.parent
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))

from frontend.app import app


@pytest.fixture
def client():
    """Flask test client fixture."""
    app.config["TESTING"] = True
    with app.test_client() as client:
        yield client


def test_index_page_loads(client):
    """Verify that the homepage loads successfully with active methods and impairment controls."""
    response = client.get("/")
    assert response.status_code == 200
    html = response.data.decode("utf-8")

    assert "TCP Header Data Modulation &amp; Signaling" in html or "TCP Header Data Modulation" in html
    assert "Sequence Number" in html
    assert "Window Size" in html
    assert "0 Bytes" in html or "0 B" in html
    assert "method-select-sequence" in html
    assert "method-select-window" in html

    # Timing signaling permanently removed
    assert "method-select-timing" not in html

    # Step 4 Network Impairment controls present
    assert "input-loss" in html
    assert "input-corrupt" in html
    assert "input-reorder" in html
    assert "input-seed" in html
    assert "packet-flow-table" in html

    # Step 5 Comparative Analysis controls present
    assert "method-select-compare" in html
    assert "comparison-matrix-table" in html


def test_api_backend_status(client):
    """Verify backend status API endpoint."""
    response = client.get("/api/backend-status")
    assert response.status_code == 200
    data = json.loads(response.data)

    assert data["available"] is True
    assert data["status"] == "READY"
    assert data["backend"] == "C"
    assert data["payload_bytes"] == 0
    assert len(data["methods"]) == 2


def test_flask_sequence_experiment_end_to_end(client):
    """
    Step 2 End-to-End Test:
    1. Flask -> C sequence experiment
    2. JSON response validation
    3. Payload strictly 0 bytes
    4. Original message == Decoded message
    """
    payload = {
        "method": "sequence",
        "message": "HELLO NETWORK"
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    # 1. Verification of JSON response format
    assert data["status"] == "success"
    assert data["method"] == "sequence"

    # 2. Verification of 0-byte payload invariant
    assert data["payload_bytes"] == 0

    # 3. Verification of exact roundtrip fidelity (original == decoded)
    assert data["original_message"] == "HELLO NETWORK"
    assert data["decoded_message"] == "HELLO NETWORK"
    assert data["reconstructed_message"] == "HELLO NETWORK"
    assert data["integrity"] == "verified"
    assert data["integrity_match"] is True

    # 4. Verification of packet counts
    assert data["packet_count"] == 104
    assert data["packets_count"] == 104
    assert data["bits_encoded"] == 104
    assert data["bit_error_rate"] == 0.0

    # 5. Verification of sequence packet telemetry
    sample_pkts = data.get("sample_packets", [])
    assert len(sample_pkts) > 0
    for pkt in sample_pkts:
        assert pkt["payload_b"] == 0
        assert pkt["seq"] >= 10100


def test_flask_window_experiment_end_to_end(client):
    """
    Step 3 End-to-End Test:
    1. Flask -> C window experiment ("HELLO NETWORK")
    2. JSON response validation
    3. Payload strictly 0 bytes
    4. Window mapping (0 -> 30000, 1 -> 60000)
    5. Original message == Decoded message
    6. Integrity status verified
    """
    payload = {
        "method": "window",
        "message": "HELLO NETWORK"
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    # 1. Verification of JSON response format
    assert data["status"] == "success"
    assert data["method"] == "window"
    assert data["method_name"] == "Window Size"

    # 2. Verification of 0-byte payload invariant
    assert data["payload_bytes"] == 0

    # 3. Verification of exact roundtrip fidelity (original == decoded)
    assert data["original_message"] == "HELLO NETWORK"
    assert data["decoded_message"] == "HELLO NETWORK"
    assert data["reconstructed_message"] == "HELLO NETWORK"
    assert data["integrity"] == "verified"
    assert data["integrity_match"] is True

    # 4. Verification of packet counts and BER
    assert data["packet_count"] == 104
    assert data["packets_count"] == 104
    assert data["bits_encoded"] == 104
    assert data["bit_error_rate"] == 0.0

    # 5. Window mapping verification
    assert data["window_mapping"]["bit_0"] == 30000
    assert data["window_mapping"]["bit_1"] == 60000

    # 6. Verification of sample packets
    sample_pkts = data.get("sample_packets", [])
    assert len(sample_pkts) > 0
    for pkt in sample_pkts:
        assert pkt["payload_b"] == 0
        assert pkt["win"] in (30000, 60000)


@pytest.mark.parametrize("method", ["sequence", "window"])
def test_api_run_experiment_success(client, method):
    """Test running active experiments via the Flask API."""
    payload = {
        "method": method,
        "message": "TEST"
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    assert data["status"] == "success"
    assert data["original_message"] == "TEST"
    assert data["reconstructed_message"] == "TEST"
    assert data["payload_bytes"] == 0


def test_api_timing_method_rejected(client):
    """Verify that timing signaling is rejected by Flask API with 400 Bad Request."""
    payload = {
        "method": "timing",
        "message": "TEST"
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 400
    data = json.loads(response.data)
    assert data["status"] == "error"
    assert "timing" in data["message"].lower()


def test_api_run_experiment_invalid_method(client):
    """Test error handling for completely invalid signaling methods."""
    payload = {
        "method": "quantum_entanglement",
        "message": "TEST"
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 400
    data = json.loads(response.data)
    assert data["status"] == "error"
    assert "Invalid signaling method" in data["message"]


def test_api_run_experiment_empty_message(client):
    """Test error handling for empty messages."""
    payload = {
        "method": "sequence",
        "message": ""
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 400
    data = json.loads(response.data)
    assert data["status"] == "error"
    assert "cannot be empty" in data["message"]


# ==============================================================================
# Step 4: Network Impairment Flask Integration Tests
# ==============================================================================

def test_flask_network_impairment_loss(client):
    """Test network packet loss via Flask API."""
    payload = {
        "method": "sequence",
        "message": "TEST NETWORK LOSS",
        "loss_percent": 15,
        "random_seed": 42
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    stats = data["packet_statistics"]
    assert stats["generated"] > 0
    assert stats["lost"] > 0
    assert data["payload_bytes"] == 0
    assert "packet_flow" in data
    assert len(data["packet_flow"]) > 0


def test_flask_network_impairment_corruption(client):
    """Test network header corruption via Flask API."""
    payload = {
        "method": "window",
        "message": "TEST CORRUPTION",
        "corruption_percent": 20,
        "random_seed": 100
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    stats = data["packet_statistics"]
    assert stats["corrupted"] > 0
    assert data["payload_bytes"] == 0
    assert "INVALID_WINDOW_VALUE" in data["detected_errors"]


def test_flask_network_impairment_reorder(client):
    """Test network packet reordering via Flask API."""
    payload = {
        "method": "sequence",
        "message": "TEST REORDERING",
        "reorder_percent": 25,
        "random_seed": 888
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    stats = data["packet_statistics"]
    assert stats["reordered"] > 0
    assert data["payload_bytes"] == 0
    assert "PACKET_ORDER_ERROR" in data["detected_errors"]


def test_flask_network_impairment_deterministic_seed(client):
    """Verify deterministic seed produces identical stats via Flask API."""
    payload = {
        "method": "sequence",
        "message": "DETERMINISTIC FLASK",
        "loss_percent": 10,
        "corruption_percent": 10,
        "reorder_percent": 10,
        "random_seed": 54321
    }
    res1 = client.post("/api/run-experiment", json=payload)
    data1 = json.loads(res1.data)

    res2 = client.post("/api/run-experiment", json=payload)
    data2 = json.loads(res2.data)

    assert data1["packet_statistics"] == data2["packet_statistics"]
    assert data1["detected_errors"] == data2["detected_errors"]


# ==============================================================================
# Step 5: Comparative Analysis Flask API Tests
# ==============================================================================

def test_flask_comparison_experiment_baseline(client):
    """Test Step 5 comparative analysis endpoint under baseline conditions."""
    payload = {
        "method": "compare",
        "message": "HELLO NETWORK",
        "loss": 0,
        "corrupt": 0,
        "reorder": 0
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    assert data["status"] == "success"
    assert data["mode"] == "comparison"
    assert data["payload_bytes"] == 0

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


def test_flask_comparison_experiment_loss(client):
    """Test comparative analysis under packet loss via Flask API."""
    payload = {
        "method": "compare",
        "message": "COMPARE LOSS",
        "loss": 15,
        "seed": 4242
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    assert data["payload_bytes"] == 0
    assert data["sequence"]["packets_lost"] > 0
    assert data["window"]["packets_lost"] > 0
    assert data["sequence"]["packets_lost"] == data["window"]["packets_lost"]


def test_flask_comparison_experiment_corruption(client):
    """Test comparative analysis under header corruption via Flask API."""
    payload = {
        "method": "compare",
        "message": "COMPARE CORRUPT",
        "corrupt": 20,
        "seed": 1111
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    assert data["payload_bytes"] == 0
    assert data["sequence"]["packets_corrupted"] > 0
    assert data["window"]["packets_corrupted"] > 0
    assert data["sequence"]["packets_corrupted"] == data["window"]["packets_corrupted"]


def test_flask_comparison_experiment_reorder(client):
    """Test comparative analysis under packet reordering via Flask API."""
    payload = {
        "method": "compare",
        "message": "COMPARE REORDER",
        "reorder": 25,
        "seed": 2222
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    assert data["payload_bytes"] == 0
    assert data["sequence"]["packets_reordered"] > 0
    assert data["window"]["packets_reordered"] > 0
    assert data["sequence"]["packets_reordered"] == data["window"]["packets_reordered"]


def test_flask_comparison_experiment_combined(client):
    """Test comparative analysis under combined impairments via Flask API."""
    payload = {
        "method": "compare",
        "message": "COMPARE COMBINED",
        "loss_percent": 10,
        "corruption_percent": 10,
        "reorder_percent": 10,
        "random_seed": 3333
    }
    response = client.post("/api/run-experiment", json=payload)
    assert response.status_code == 200
    data = json.loads(response.data)

    assert data["payload_bytes"] == 0
    assert data["sequence"]["packets_generated"] == data["window"]["packets_generated"]
    assert data["sequence"]["packets_lost"] == data["window"]["packets_lost"]
    assert data["sequence"]["packets_corrupted"] == data["window"]["packets_corrupted"]
    assert data["sequence"]["packets_reordered"] == data["window"]["packets_reordered"]

