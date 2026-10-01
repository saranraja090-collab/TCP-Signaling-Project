# TCP Header Data Modulation and Signaling — C Networking Backend

### Computer Networks Academic Experimental Project

---

## 1. Overview

The **C Networking Backend** is the core transport-layer simulation engine for the **TCP Header Data Modulation and Signaling** project. It models and investigates transport-layer signaling techniques where arbitrary information is modulated strictly into **TCP packet attributes and metadata** rather than traditional application payloads.

### Crucial Technical Principles:
1. **0-Byte Application Payload Invariant (`Payload = 0 bytes`):**  
   In standard TCP communication, application data resides in the payload, while the TCP header carries transport-layer control state. In this project, all simulated packets have a payload length of strictly 0 bytes. All information transfer occurs through header and timing modulation.
2. **Experimental Signaling/Modulation Only:**  
   TCP header fields (Sequence Numbers, Advertised Window Sizes) have real protocol meanings defined in RFC 793 / RFC 9293. They are **not arbitrary storage containers**. The project models their behavior as an experimental signaling technique in a controlled simulation environment.
3. **Simulation-Based:**  
   Execution is performed in-memory without raw sockets or elevated privileges, ensuring safe, ethical, and deterministic experimentation.

---

## 2. Simulated Packet Model (`simulated_packet_t`)

Defined in [include/packet.h](include/packet.h):

```c
typedef struct {
    /* --- Conceptual TCP Header Fields (RFC 793 / 9293) --- */
    uint16_t src_port;               /* Conceptual TCP source port */
    uint16_t dst_port;               /* Conceptual TCP destination port */
    uint32_t sequence_number;        /* Conceptual TCP Sequence Number (Method 1) */
    uint32_t acknowledgment_number;  /* Conceptual TCP ACK Number */
    uint16_t window_size;            /* Conceptual Advertised Window Size (Method 2) */
    uint8_t  flags;                  /* Conceptual Control Flags (SYN, ACK, FIN, etc.) */
    uint16_t payload_length;         /* Application Payload Length: ALWAYS 0 BYTES */

    /* --- Simulation & Experimental Instrumentation Metadata --- */
    uint32_t packet_id;              /* Simulation sequence index (0, 1, 2...) */
    double   timestamp_ms;           /* Monotonic departure timestamp */
    double   inter_packet_gap_ms;    /* Inter-packet delay delta t (Method 3) */
    signaling_method_t signaling_method; /* Signaling method under test */
    uint8_t  signaled_symbol;        /* Symbol/bit modulated in this packet */
} simulated_packet_t;
```

---

## 3. The Active Experimental Methods

| Method | Target Mechanism | In-Memory Simulation Modeling | Payload | Status |
|---|---|---|---|---|
| **1. Sequence Number** | TCP `sequence_number` (32 bits) | Modulates sequence increments/offsets ($\Delta \text{seq} = +100 / +200$) | 0 bytes | **ACTIVE (Verified)** |
| **2. Window Size** | TCP `window_size` (16 bits) | Modulates discrete window size steps ($W = 30000 / 60000\text{ B}$) | 0 bytes | **ACTIVE (Verified)** |
| — *Timing* | Departure delay ($\Delta t$) | *Permanently removed from project scope* | 0 bytes | *OUT OF SCOPE* |

---

## 4. Directory Structure

```
backend/
├── include/
│   ├── encoder.h            # Text to binary bitstream translation
│   ├── error_control.h      # CRC-16 and Hamming(7,4) prototypes
│   ├── packet.h             # Simulated TCP packet data structure (Payload = 0 B)
│   ├── sequence_method.h    # Sequence-number signaling definitions
│   ├── window_method.h      # Window-size signaling definitions
│   ├── network_simulator.h  # Step 4 Network impairment layer
│   ├── comparison.h         # Step 5 Comparative analysis engine
│   ├── decoder.h            # Receiver bitstream reassembly into text
│   ├── verification.h       # BER and integrity check functions
│   └── unit_tests.h         # Comprehensive C unit tests (46 tests)
│
├── src/
│   ├── main.c               # CLI driver and JSON IPC responder
│   ├── encoder.c            # Bitstream encoder implementation
│   ├── error_control.c      # CRC-16 and Hamming(7,4) implementation
│   ├── packet.c             # Packet initialization and formatting
│   ├── sequence_method.c    # Sequence modulation & demodulation
│   ├── window_method.c      # Window modulation & demodulation
│   ├── network_simulator.c  # In-memory network impairment pipeline
│   ├── comparison.c         # Comparative analysis engine implementation
│   ├── decoder.c            # Decoder implementation
│   ├── verification.c       # Verification & BER implementation
│   └── unit_tests.c         # 46 native unit tests
│
├── bin/                     # Output directory for compiled executables
│   └── backend_engine.exe   # Compiled C simulation engine & CLI
├── Makefile                 # Build automation (all, run, test, clean)
└── README.md
```

---

## 5. Build and Execution Instructions

### Prerequisites
- GCC compiler (MinGW / Dev-Cpp GCC with C99 support)
- GNU Make

### Commands
```bash
# Build the backend executable
make all

# Run unit tests (46 tests)
make test

# Clean build artifacts
make clean
```

---

## 6. Python Web Frontend Interoperability (CLI / Subprocess IPC)

The C backend communicates with the Python Flask frontend via standard subprocess execution with JSON input/output:

### Status Query:
```bash
./bin/backend_engine.exe --json
```
Output:
```json
{
  "backend": "C",
  "status": "READY",
  "execution_mode": "In-Memory Simulation",
  "payload_bytes": 0,
  "methods": [
    {"id": "sequence", "name": "Sequence Number", "field": "TCP sequence_number (32-bit)"},
    {"id": "window", "name": "Window Size", "field": "TCP window_size (16-bit)"}
  ]
}
```

### Running an Individual Experiment:
```bash
./bin/backend_engine.exe -m sequence -d "HELLO" --loss 5 --seed 42 --json
```

### Running Comparative Analysis:
```bash
./bin/backend_engine.exe -m compare -d "HELLO NETWORK" --loss 5 --corrupt 5 --reorder 5 --seed 12345 --json
```
Output:
```json
{
  "status": "success",
  "mode": "comparison",
  "experiment": {
    "message": "HELLO NETWORK",
    "loss_percent": 5,
    "corruption_percent": 5,
    "reorder_percent": 5,
    "random_seed": 12345
  },
  "payload_bytes": 0,
  "sequence": {
    "method": "sequence",
    "packets_generated": 104,
    "packets_received": 99,
    "packets_lost": 5,
    "packets_corrupted": 5,
    "packets_reordered": 5,
    "errors_detected": 10,
    "decoded_message": "...",
    "integrity_match": false,
    "integrity": "corrupted"
  },
  "window": {
    "method": "window",
    "packets_generated": 104,
    "packets_received": 99,
    "packets_lost": 5,
    "packets_corrupted": 5,
    "packets_reordered": 5,
    "errors_detected": 10,
    "decoded_message": "...",
    "integrity_match": false,
    "integrity": "corrupted"
  }
}
```
