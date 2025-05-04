Selective Repeat Protocol Implementation
This project implements the Selective Repeat (SR) protocol by modifying an existing Go-Back-N (GBN) implementation. The implementation works with a network simulator that can introduce packet loss and corruption.
Files

sr.c - The Selective Repeat protocol implementation
sr.h - Header file for the Selective Repeat protocol
emulator.c - Network simulator (provided)
emulator.h - Header file for network simulator (provided)

Implementation Details
Key Features

Window Management:

The sender maintains a window of size 8
The receiver also maintains a window of size 8
Only transmits packets within the window


Selective Acknowledgment and Retransmission:

Each packet is individually acknowledged
Only lost or corrupted packets are retransmitted
Timer is maintained for the oldest unacknowledged packet


Buffer Management:

The sender buffers all packets in the window
The receiver buffers out-of-order packets
Packets are delivered in-order to the application layer


Error Handling:

Checksums for detecting packet corruption
Timers for handling packet loss
Duplicate packet detection and handling



Key Data Structures

sndpkt[] - Array to store packets for potential retransmission
sndpktValid[] - Array to track valid packets in the buffer
sndpktAcked[] - Array to track acknowledged packets
rcvpkt[] - Array to store received packets
rcvpktValid[] - Array to track valid received packets
base - Base of the sender window
nextseqnum - Next sequence number to use
expectedseqnum - Next expected sequence number at receiver

Compilation and Running
To compile:
gcc -Wall -ansi -pedantic -o sr emulator.c sr.c
To run:
./sr
You will be prompted to enter simulation parameters:

Number of messages to simulate
Packet loss probability
Packet corruption probability
Average time between messages
Trace level (recommended: 2 for debugging)

Development Process and Commit History
Initial Setup

Created sr.c and sr.h files based on the provided gbn.c and gbn.h
Set up basic structures and constants for SR protocol

Window Management Implementation

Modified window management to handle SR window (size 8)
Implemented separate packet tracking for sending and receiving

ACK Handling

Updated A_input() to handle selective acknowledgments
Implemented individual packet acknowledgment

Retransmission Logic

Modified A_timerinterrupt() to retransmit only specific packets
Implemented timer management for multiple packets

Buffer Management

Added buffering for out-of-order packets at receiver
Implemented logic to deliver in-order packets to layer 5

Testing and Debugging

Added debug output to track protocol behavior
Fixed issues with window advancement
Fixed issues with ACK handling
Fixed issues with buffer management

Final Optimizations

Fine-tuned timer management
Improved error handling
Optimized buffer usage

Differences from Go-Back-N

Window Management:

GBN: Only sender maintains a window
SR: Both sender and receiver maintain windows


ACK Handling:

GBN: Cumulative ACKs
SR: Individual packet ACKs


Retransmission:

GBN: Retransmits all packets from lost packet onward
SR: Retransmits only specific lost packets


Buffer Management:

GBN: Receiver discards out-of-order packets
SR: Receiver buffers out-of-order packets
