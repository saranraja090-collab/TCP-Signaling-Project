# TCP Header Data Modulation and Signaling
### A Computer Networks Academic Experimental Project

---

## 1. Project Overview

**TCP Header Data Modulation and Signaling** is an educational Computer Networks project designed to experimentally explore how arbitrary digital information can be represented, transmitted, and recovered using **TCP packet characteristics and metadata** rather than traditional application payloads.

In standard network communication, the TCP header carries control metadata (for sequencing, flow control, and connection state), while user data resides in the application payload. In this project, packets are transmitted with **zero-byte payloads (`Payload = 0 bytes`)**. All information is signaled through carefully modulated packet attributes.

---

## 2. Why is this a Computer Networks Project?

This project explores foundational concepts across multiple layers of network engineering:

1. **TCP Architecture & Protocol Header Mechanics:**  
   Understanding exact bit allocations, endianness, field semantics (Sequence Numbers, Window Sizes, Flags, Offsets), and protocol invariants defined in RFC 793 / RFC 9293.

2. **Signaling and Covert Channels:**  
   Investigating how transport-layer protocol fields and packet transmission timing can act as signaling channels.

3. **Flow Control & Congestion Control Semantics:**  
   Analyzing how modulating fields like the TCP Window Size interacts with protocol state expectations.

4. **Network Traffic Analysis & Anomaly Detection:**  
   Learning how modern firewalls, Intrusion Detection Systems (IDS), and Deep Packet Inspection (DPI) mechanisms analyze entropy, statistical distributions, and timing jitter to identify non-standard protocol usage.

---

## 3. Active Experimental Signaling Methods

The project investigates transport-layer header modulation across two primary active methods (Timing-based signaling has been permanently removed from project scope):

| # | Experimental Method | Target Header Field | Signaling Modulation Rule | Status |
|---|---------------------|---------------------|---------------------------|--------|
| **1** | **Sequence-Number-Based Signaling** | TCP `sequence_number` (32-bit) | $\Delta \text{seq} = +100$ (bit `0`), $\Delta \text{seq} = +200$ (bit `1`) | **ACTIVE (Verified)** |
| **2** | **Window-Size-Based Signaling** | TCP `window_size` (16-bit) | $W = 30000\text{ B}$ (bit `0`), $W = 60000\text{ B}$ (bit `1`) | **ACTIVE (Verified)** |
| — | *Packet Timing-Based Signaling* | Inter-packet delay ($\Delta t$) | *Permanently removed from project scope* | *REMOVED* |

---

## 4. Normal TCP vs. Experimental Signaling

### Traditional TCP Flow:
```text
+-------------------------------------------------------+
| Application Data (HTTP, Email, File Transfer, etc.)    |
+-------------------------------------------------------+
                           ↓
+-------------------------------------------------------+
| Transport Layer (TCP)                                 |
|  - Generates 20+ byte TCP Header (Ports, Seq, Ack)    |
|  - Appends Application Payload                        |
+-------------------------------------------------------+
                           ↓
+-------------------------------------------------------+
| Network Layer (IP) & Physical Wire                    |
|  - Total Packet = IP Header + TCP Header + Payload    |
+-------------------------------------------------------+
```

### Experimental Signaling Flow:
```text
+-------------------------------------------------------+
| Original Secret/Signaling Message                     |
| Example: "HELLO" (ASCII -> Binary: 01001000...)       |
+-------------------------------------------------------+
                           ↓
+-------------------------------------------------------+
| Encoder & Modulator                                   |
|  - Maps bits into chosen packet characteristic:       |
|    * Sequence Number Offsets                          |
|    * Window Size Steps                                |
|    * Inter-packet Transmission Delays                 |
+-------------------------------------------------------+
                           ↓
+-------------------------------------------------------+
| Simulated TCP Packets (Payload = 0 bytes)             |
|  - Every packet carries strictly control metadata!   |
+-------------------------------------------------------+
                           ↓
+-------------------------------------------------------+
| Receiver & Demodulator                                |
|  - Measures packet characteristics                    |
|  - Recovers binary stream and reconstructs message    |
+-------------------------------------------------------+
                           ↓
+-------------------------------------------------------+
| Verification (Bitwise Text Match & Integrity Check)   |
+-------------------------------------------------------+
```

---

## 5. TCP Header vs. Payload: Crucial Distinction

- **TCP Header (Control Plane):**  
  A 20-byte to 60-byte metadata structure placed at the front of every TCP segment. It contains source and destination port numbers, sequence numbers, acknowledgment numbers, flags (SYN, ACK, FIN, RST, PSH, URG), window sizes, and checksums. These fields ensure reliability, ordering, and flow control.

- **TCP Payload (Data Plane):**  
  The actual user data being delivered to the destination application process.

> **Important Technical Note:**  
> TCP header fields are **not arbitrary storage containers**. Sequence numbers maintain byte-stream ordering and handle retransmissions; window sizes throttle sender transmission to prevent buffer overflow. This project models their use as an **experimental modulation technique** within a controlled simulation to study detection, constraints, and channel capacity.

---

## 6. Why Initial Development is Simulation-Only

1. **Safety & Ethics:**  
   Arbitrary manipulation of live TCP fields over public networks can break connections, violate network access policies, or cause unintended network interference.
2. **Platform Independence:**  
   Raw socket transmission on Windows and Unix requires elevated administrator/root privileges and custom OS drivers (e.g., WinPcap / Npcap). Simulation runs smoothly on standard Python 3 on any operating system without root privileges.
3. **Controlled Experimental Variables:**  
   Simulated network channels allow precise injection of latency, jitter, packet loss, and reordering, allowing systematic evaluation of decoding accuracy.
4. **Pedagogical Clarity:**  
   Beginners can focus on bitwise operations, modulation logic, and protocol state concepts without fighting low-level OS networking quirks.

---

## 7. Migrated System Architecture

```text
=============================================================================
                    MIGRATED ARCHITECTURE (C BACKEND + FLASK)
=============================================================================

                    WEB BROWSER (Dashboard)
                               |
                               | HTTP (REST API)
                               v
                    +----------------------+
                    | Python Web Frontend  |
                    | Flask Gateway        |
                    +----------+-----------+
                               |
                               | Subprocess IPC (JSON)
                               v
                    +----------------------+
                    |      C BACKEND       |
                    |                      |
                    | Message Encoding     |
                    | Error Control        |
                    | Packet Modeling      |
                    | Sequence Modulation  |
                    | Window Modulation    |
                    +----------+-----------+
                               |
                               v
              +----------------------------------+
              |   SIMULATED NETWORK LAYER (C)    |
              | 1. Packet Generation             |
              | 2. Packet Header Corruption      |
              | 3. Packet Loss                   |
              | 4. Packet Reordering             |
              | (Deterministic PRNG / 0B Payload)|
              +----------------+-----------------+
                               |
                               v
                    +----------------------+
                    |      C RECEIVER      |
                    | Anomaly Detection    |
                    | Symbol Demodulation  |
                    | Bitstream Decoding   |
                    | Integrity / BER      |
                    +----------------------+
```

---

## 8. Step 4 — Network Impairment & Error Simulation Layer

The C backend features an in-memory network simulator (`network_simulator.h` / `network_simulator.c`) executing between modulation and receiver ingestion.

### Pipeline Execution Order
1. **Packet Generation:** Packets are modulated with signaling metadata and strictly 0 bytes of application payload.
2. **Packet Corruption:** Controlled header corruption alters sequence deltas or advertised window sizes while preserving `payload_length == 0`.
3. **Packet Loss:** Drops simulated packets based on the loss probability parameter.
4. **Packet Reordering:** Inverts adjacent packet delivery order to simulate out-of-order arrival.
5. **Receiver Ingestion & Anomaly Detection:** The receiver analyzes arrivals for protocol irregularities:
   - `INVALID_SEQUENCE_DELTA`: Detected when $\Delta \text{seq} \notin \{+100, +200\}$.
   - `INVALID_WINDOW_VALUE`: Detected when `window_size` is outside valid symbol thresholds.
   - `PACKET_ORDER_ERROR`: Detected when sequence numbers arrive out of monotonic order.
   - `PACKET_LOSS_DISRUPTION`: Detected when sequence delta indicates missing intermediate segments.

### Strict 0-Byte Payload Invariant
Every simulated packet carries `payload_length == 0` and `payload_bytes == 0` at all times:
- Before modulation
- Inside modulation algorithms
- Across network corruption, loss, and reordering
- Upon receiver demodulation and verification

---

## 9. Repository Structure

```text
TCP-Signaling-Project/
│
├── backend/                        # High-Performance C Networking Backend
│   ├── include/                    # C Header Files
│   │   ├── encoder.h               # Text to bitstream translation
│   │   ├── error_control.h         # CRC-16 and Hamming(7,4) prototypes
│   │   ├── packet.h                # Simulated TCP packet data model (Payload = 0 B)
│   │   ├── sequence_method.h       # Sequence-number signaling definitions
│   │   ├── window_method.h         # Window-size signaling definitions
│   │   ├── network_simulator.h     # Step 4 Network impairment layer
│   │   ├── comparison.h            # Step 5 Comparative analysis engine
│   │   ├── decoder.h               # Receiver decoding & reassembly
│   │   ├── verification.h          # Message integrity & Bit Error Rate
│   │   └── unit_tests.h            # Comprehensive C unit tests (46 tests)
│   │
│   ├── src/                        # C Source Files
│   │   ├── main.c                  # CLI demonstration & JSON IPC handler
│   │   ├── encoder.c
│   │   ├── error_control.c
│   │   ├── packet.c
│   │   ├── sequence_method.c
│   │   ├── window_method.c
│   │   ├── network_simulator.c     # In-memory network impairment pipeline
│   │   ├── comparison.c            # Comparative analysis execution & formatters
│   │   ├── decoder.c
│   │   ├── verification.c
│   │   └── unit_tests.c
│   │
│   ├── bin/                        # Output directory for compiled executables
│   │   └── backend_engine.exe      # C backend CLI & simulation engine
│   │
│   ├── Makefile                    # Build automation (all, test, clean)
│   └── README.md                   # C backend documentation
│
├── frontend/                       # Python Flask Web Frontend & API Gateway
│   ├── app.py                      # Flask application and subprocess coordinator
│   ├── requirements.txt            # Flask dependencies
│   ├── templates/
│   │   └── index.html              # Modern research dashboard with comparative analysis
│   └── static/
│       ├── css/
│       │   └── style.css           # Premium dark-mode design system & telemetry styles
│       └── js/
│           └── main.js             # Client controller, presets, and comparison matrix
│
├── archive/                        # Preserved Legacy Codebase
│   └── legacy_python_backend/      # Original Step 1 & 2 Python implementation
│
├── tests/                          # Unified Automated Test Suite (Pytest)
│   ├── conftest.py                 # Pytest environment bootstrap
│   ├── test_c_backend.py           # Subprocess tests for C executable (20 tests)
│   ├── test_frontend.py            # HTTP, API, & comparison integration tests (18 tests)
│   ├── test_encoder_decoder.py     # Encoder/decoder and CRC/Hamming tests (22 tests)
│   └── test_smoke.py               # Smoke & structure validation tests (4 tests)
│
├── captures/                       # Directory for simulated packet trace exports
├── results/                        # Experimental benchmarking reports & graphs
├── docs/                           # Project technical documentation
├── requirements.txt                # Unified Python requirements
└── README.md                       # Master project documentation
```

---

## 10. Step 5 — Comparative Analysis Engine (Sequence vs. Window)

The project includes an empirical **Comparative Analysis Engine** that benchmarks **Sequence Number Signaling** directly against **Window Size Signaling** under identical experimental conditions.

### 1. Conceptual Framework & Nature of Simulation
This project operates as an:
> **"In-memory discrete simulation of experimental signaling through TCP header metadata."**

It models transport-layer packet abstractions in memory without deploying raw sockets or injecting packets onto physical network interfaces.

### 2. Experimental Methodology & Variable Isolation
To uphold scientific validity and eliminate confounding factors:
- **Independent Variable:** Signaling Method under evaluation (`Sequence Number` vs. `Window Size`).
- **Controlled Variables:**
  - Original Message string (identical byte sequence and bit representation).
  - Network Impairment Probabilities: Loss %, Corruption %, Reordering %.
  - Pseudo-Random Generator (PRNG) Seed (`--seed`): Identical deterministic seed supplied to both pipelines.
  - Zero-Byte Application Payload Invariant: Strictly `0 bytes` (`payload_length == 0`) across all packets.
- **Why the Same Seed is Mandatory:**  
  Using distinct seeds would subject the two methods to disparate random impairment events (e.g., method A losing packet 2 while method B loses packet 15). Supplying the identical seed ensures that both methods encounter identical pseudo-random stochastic decisions on packet boundaries.
- **No Subjective Ranking:**  
  The engine does **not** designate a "winner", "best method", or "superiority score". It reports empirical measurements and integrity outcomes for academic evaluation.

### 3. Measured Experimental Metrics
For each signaling method, the engine records:
- `packets_generated`: Total simulated TCP segments emitted.
- `packets_received`: Number of segments delivered through the impairment channel.
- `packets_lost`: Number of segments discarded by the loss simulator.
- `packets_corrupted`: Number of segment headers altered by the corruption simulator.
- `packets_reordered`: Number of segments inverted by the reordering simulator.
- `payload_bytes`: Verified payload volume (strictly `0 bytes`).
- `errors_detected`: Protocol anomalies detected by receiver validation.
- `decoded_message`: Text recovered by demodulation and symbol reconstruction.
- `integrity_status`: Binary verification (`VERIFIED` vs `CORRUPTED`).

### 4. Structured Comparison Matrix
Example experimental matrix for `HELLO NETWORK`:

| Metric | Sequence Number Signaling | Window Size Signaling |
|---|---|---|
| **Signaling Target** | TCP `sequence_number` (32-bit) | TCP `window_size` (16-bit) |
| **Packets Generated** | 104 | 104 |
| **Packets Received** | 104 | 104 |
| **Packets Lost** | 0 | 0 |
| **Packets Corrupted** | 0 | 0 |
| **Packets Reordered** | 0 | 0 |
| **Payload Bytes** | **0 B** | **0 B** |
| **Errors Detected** | 0 | 0 |
| **Decoded Message** | `HELLO NETWORK` | `HELLO NETWORK` |
| **Integrity Status** | **VERIFIED** | **VERIFIED** |

### 5. Comparison JSON Telemetry Structure
When executed with `--json`, the engine outputs a standardized JSON document:
```json
{
  "status": "success",
  "mode": "comparison",
  "experiment": {
    "message": "HELLO NETWORK",
    "loss_percent": 0,
    "corruption_percent": 0,
    "reorder_percent": 0,
    "random_seed": 12345
  },
  "payload_bytes": 0,
  "sequence": {
    "method": "sequence",
    "packets_generated": 104,
    "packets_received": 104,
    "packets_lost": 0,
    "packets_corrupted": 0,
    "packets_reordered": 0,
    "errors_detected": 0,
    "decoded_message": "HELLO NETWORK",
    "integrity_match": true,
    "integrity": "verified"
  },
  "window": {
    "method": "window",
    "packets_generated": 104,
    "packets_received": 104,
    "packets_lost": 0,
    "packets_corrupted": 0,
    "packets_reordered": 0,
    "errors_detected": 0,
    "decoded_message": "HELLO NETWORK",
    "integrity_match": true,
    "integrity": "verified"
  }
}
```

---

## 11. Build, Run, and Testing Instructions

### 1. Compile and Test the C Backend
```bash
# Navigate to the C backend directory
cd backend

# Compile backend_engine.exe
make all

# Run all 46 native C unit tests
make test
```

### 2. Run CLI Experiments

#### Individual Method Runs:
```bash
# Normal sequence signaling
backend/bin/backend_engine.exe -m sequence -d "HELLO NETWORK"

# Normal window signaling
backend/bin/backend_engine.exe -m window -d "HELLO NETWORK"

# Sequence signaling with 10% packet loss and deterministic seed
backend/bin/backend_engine.exe -m sequence -d "HELLO" --loss 10 --seed 42
```

#### Comparison Mode Runs:
```bash
# Baseline normal comparison (0% loss, 0% corruption, 0% reorder)
backend/bin/backend_engine.exe -m compare -d "HELLO NETWORK"

# Comparison under 10% packet loss
backend/bin/backend_engine.exe -m compare -d "HELLO NETWORK" --loss 10 --seed 12345

# Comparison under 10% header corruption
backend/bin/backend_engine.exe -m compare -d "HELLO NETWORK" --corrupt 10 --seed 12345

# Comparison under 10% packet reordering
backend/bin/backend_engine.exe -m compare -d "HELLO NETWORK" --reorder 10 --seed 12345

# Comparison under combined network impairments with JSON output
backend/bin/backend_engine.exe -m compare -d "HELLO NETWORK" --loss 5 --corrupt 5 --reorder 5 --seed 12345 --json
```

### 3. Start the Python Web Frontend
```bash
# From the project root
python frontend/app.py
```
Open your browser at `http://127.0.0.1:5000` to access the interactive dashboard with the **[ Compare Both ]** mode, condition presets (Normal, Packet Loss, Corruption, Reordering, Combined), and the side-by-side comparative matrix.

### 4. Run Automated Pytest Suite
```bash
# Run all 64 Python and integration tests
python -m pytest -v
```
