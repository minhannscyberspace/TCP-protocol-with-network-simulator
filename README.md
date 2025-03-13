# Reliable Network Protocol Simulator 


A TCP-inspired implementation of a reliable data transfer protocol with dynamic timeout management and network condition simulation. Handles packet loss, delays, and out-of-order delivery while maintaining high data integrity.

## Features 

- **Sliding Window Protocol** (Window Size = 5)
-  **Adaptive Timeout Calculation** (RFC 6298 compliant)
- **Smart Retransmission Logic** with exponential backoff
- **Network Simulation**:
  - Configurable packet loss (0-100%)
  - Programmable network delays
  - Dummy packet injection for buffer stress tests
- **Out-of-Order Handling** with sequence validation
- **Real-Time Statistics** (RTT, retransmissions, throughput)

## Architecture Overview 

1. Sliding window controller manages unacknowledged packets with implementation of Go-Back-N ARQ strategy. 

2. RTT Estimator where  RTO = SRTT + max(G, K*RTTVAR) // RFC 6298 Implementation

3. Network Simulator

## Installation 
Clone repository:
git clone https://github.com/minhannscyberspace/TCP-protocol-with-network-simulator.git

cd TCP-protocol-with-network-simulator

Or downloading the associated .zip file. 

License: MIT © 2024 Minh Nguyen

Report Issues: GitHub Issues
