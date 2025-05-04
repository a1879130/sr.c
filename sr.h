/*
 * Selective Repeat Protocol Implementation
 * Header file
 */

#ifndef SR_H
#define SR_H

/* Function prototypes */
void A_output(struct msg message);
void A_input(struct pkt packet);
void A_timerinterrupt(void);
void A_init(void);
void B_input(struct pkt packet);
void B_init(void);
void B_timerinterrupt(void);

/* Helper function prototypes */
int calculateChecksum(struct pkt packet);
int isChecksumValid(struct pkt packet);

/* Constants */
#define A 0
#define B 1

#endif /* SR_H */
