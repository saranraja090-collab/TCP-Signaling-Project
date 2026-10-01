"""
Network Simulation Subsystem
============================
Provides an abstracted transmission medium between sender and receiver.
In the initial development phase, this operates entirely in-memory as a discrete
packet simulation with optional configurable channel impairment (latency, jitter).

Architecture Note:
------------------
Keeping this layer modular ensures that in later phases, this simulation interface
can be connected to a controlled laboratory network layer (or Scapy/raw sockets)
without modifying the core encoding and decoding logic.
"""
