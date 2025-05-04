/*
 * Selective Repeat Protocol Implementation
 * Based on the provided Go-Back-N implementation
 * 
 * This file implements the Selective Repeat reliable data transfer protocol.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emulator.h"
#include "sr.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
#define WINDOWSIZE 8       /* window size for Selective Repeat protocol */
#define BUFSIZE 50         /* buffer size for storing packets */
#define RTT 16.0           /* round trip time - specified in assignment */

struct pkt sndpkt[BUFSIZE];    /* array of packets to be sent */
int sndpktValid[BUFSIZE];      /* 1 if packet is valid, 0 otherwise */
int sndpktAcked[BUFSIZE];      /* 1 if packet has been ACKed, 0 otherwise */
struct pkt rcvpkt[BUFSIZE];    /* array of received packets */
int rcvpktValid[BUFSIZE];      /* 1 if packet is valid, 0 otherwise */
int base;                      /* base of the window */
int nextseqnum;                /* next sequence number to use */
int expectedseqnum;            /* next expected sequence number at receiver */
int timerActive;               /* 1 if timer is active, 0 otherwise */
int timerForPacket;            /* sequence number of packet for which timer is running */
int lastAckReceived;           /* last ACK received */

FILE *fp;                      /* for debugging */

/* 
 * Calculates checksum for a packet
 */
int calculateChecksum(struct pkt packet) {
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    
    int i;
    for (i = 0; i < 20; i++) {
        checksum += (unsigned char)packet.payload[i];
    }
    
    return checksum;
}

/* 
 * Checks if a packet's checksum is valid
 */
int isChecksumValid(struct pkt packet) {
    int calculatedChecksum = calculateChecksum(packet);
    return (calculatedChecksum == packet.checksum);
}

/* 
 * Initialize sender variables
 */
void A_init() {
    base = 1;
    nextseqnum = 1;
    timerActive = 0;
    
    int i;
    for (i = 0; i < BUFSIZE; i++) {
        sndpktValid[i] = 0;
        sndpktAcked[i] = 0;
    }
    
    /* Open debug file if needed */
    fp = fopen("debug.txt", "w");
    if (fp == NULL) {
        printf("Error opening debug file\n");
    }
}

/* 
 * Initialize receiver variables
 */
void B_init() {
    expectedseqnum = 1;
    
    int i;
    for (i = 0; i < BUFSIZE; i++) {
        rcvpktValid[i] = 0;
    }
}

/* 
 * Called from layer 5, passed the data to be sent to other side 
 */
void A_output(struct msg message) {
    /* If next sequence number is within window size */
    if (nextseqnum < base + WINDOWSIZE) {
        /* Create packet */
        struct pkt packet;
        packet.seqnum = nextseqnum;
        packet.acknum = 0;  /* Not used for data packets */
        memcpy(packet.payload, message.data, 20);
        packet.checksum = calculateChecksum(packet);
        
        /* Store packet for potential retransmission */
        sndpkt[nextseqnum % BUFSIZE] = packet;
        sndpktValid[nextseqnum % BUFSIZE] = 1;
        sndpktAcked[nextseqnum % BUFSIZE] = 0;
        
        /* Send packet to layer 3 */
        tolayer3(A, packet);
        
        /* Start timer if not already running */
        if (!timerActive) {
            starttimer(A, RTT);
            timerActive = 1;
            timerForPacket = nextseqnum;
        }
        
        /* Increment sequence number */
        nextseqnum++;
        
        if (fp != NULL) {
            fprintf(fp, "A_output: sent packet with seqnum %d\n", packet.seqnum);
            fflush(fp);
        }
    } else {
        /* Window is full, reject message */
        if (fp != NULL) {
            fprintf(fp, "A_output: window full, rejecting message\n");
            fflush(fp);
        }
    }
}

/* 
 * Called from layer 3, when a packet arrives for layer 4 at A
 */
void A_input(struct pkt packet) {
    /* Check if packet is valid */
    if (isChecksumValid(packet)) {
        int acknum = packet.acknum;
        
        if (fp != NULL) {
            fprintf(fp, "A_input: received valid ACK for packet %d\n", acknum);
            fflush(fp);
        }
        
        /* Mark packet as ACKed */
        sndpktAcked[acknum % BUFSIZE] = 1;
        
        /* If it's the base packet, advance window */
        if (acknum == base) {
            /* Move base forward to the next unacknowledged packet */
            while (sndpktAcked[base % BUFSIZE] && sndpktValid[base % BUFSIZE]) {
                sndpktValid[base % BUFSIZE] = 0;  /* Packet no longer needed */
                base++;
            }
        }
        
        /* If all packets have been ACKed, stop timer */
        int allAcked = 1;
        int i;
        for (i = base; i < nextseqnum; i++) {
            if (!sndpktAcked[i % BUFSIZE]) {
                allAcked = 0;
                break;
            }
        }
        
        if (allAcked) {
            stoptimer(A);
            timerActive = 0;
        } else if (acknum == timerForPacket) {
            /* If ACK was for packet with timer, restart timer for next unACKed packet */
            stoptimer(A);
            
            /* Find first unACKed packet */
            for (i = base; i < nextseqnum; i++) {
                if (!sndpktAcked[i % BUFSIZE]) {
                    timerForPacket = i;
                    starttimer(A, RTT);
                    timerActive = 1;
                    break;
                }
            }
        }
    } else {
        /* Invalid packet, ignore */
        if (fp != NULL) {
            fprintf(fp, "A_input: received invalid ACK\n");
            fflush(fp);
        }
    }
}

/* 
 * Called when A's timer goes off 
 */
void A_timerinterrupt() {
    if (fp != NULL) {
        fprintf(fp, "A_timerinterrupt: retransmitting packet %d\n", timerForPacket);
        fflush(fp);
    }
    
    /* Retransmit only the packet for which timer expired */
    if (timerForPacket >= base && timerForPacket < nextseqnum) {
        struct pkt packet = sndpkt[timerForPacket % BUFSIZE];
        tolayer3(A, packet);
    }
    
    /* Reset timer */
    starttimer(A, RTT);
    timerActive = 1;
}

/* 
 * Called from layer 3, when a packet arrives for layer 4 at B
 */
void B_input(struct pkt packet) {
    /* Check if packet is valid */
    if (isChecksumValid(packet)) {
        int seqnum = packet.seqnum;
        
        /* Create ACK packet */
        struct pkt ackpkt;
        ackpkt.acknum = seqnum;
        ackpkt.seqnum = 0;  /* Not used for ACK packets */
        memset(ackpkt.payload, 0, 20);
        ackpkt.checksum = calculateChecksum(ackpkt);
        
        /* Send ACK packet */
        tolayer3(B, ackpkt);
        
        if (fp != NULL) {
            fprintf(fp, "B_input: received valid packet with seqnum %d, sending ACK\n", seqnum);
            fflush(fp);
        }
        
        /* If packet is within receive window */
        if (seqnum >= expectedseqnum && seqnum < expectedseqnum + WINDOWSIZE) {
            /* Buffer packet */
            rcvpkt[seqnum % BUFSIZE] = packet;
            rcvpktValid[seqnum % BUFSIZE] = 1;
            
            /* If all packets up to a certain point have been received, deliver them */
            while (rcvpktValid[expectedseqnum % BUFSIZE]) {
                /* Deliver packet to layer 5 */
                tolayer5(B, rcvpkt[expectedseqnum % BUFSIZE].payload);
                rcvpktValid[expectedseqnum % BUFSIZE] = 0;
                expectedseqnum++;
            }
        } else if (seqnum >= expectedseqnum - WINDOWSIZE && seqnum < expectedseqnum) {
            /* Packet is a duplicate of one already received and ACKed */
            if (fp != NULL) {
                fprintf(fp, "B_input: duplicate packet %d\n", seqnum);
                fflush(fp);
            }
        }
    } else {
        /* Invalid packet, ignore */
        if (fp != NULL) {
            fprintf(fp, "B_input: received invalid packet\n");
            fflush(fp);
        }
    }
}

/* 
 * Called when B's timer goes off 
 */
void B_timerinterrupt() {
    /* Not used in this implementation */
}
