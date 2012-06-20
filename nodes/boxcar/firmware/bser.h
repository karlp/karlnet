/* 
 * File:   bser.h
 * Author: karlp
 * Buffered serial helper stuff
 *
 * Created on June 6, 2012, 3:26 PM
 */

#ifndef RMESERIAL_H
#define	RMESERIAL_H

#define SERIAL_BUFFER_SIZE 64

struct ring_buffer {
    unsigned char buffer[SERIAL_BUFFER_SIZE];
    volatile unsigned int head;
    volatile unsigned int tail;
};

/**
 * Handles any init, so you don't need to memset or anything, just make sure it's alloced
 * @param _rx_buffer
 */
void bser_begin(struct ring_buffer *_rx_buffer);

/**
 * Call this from your rx interrupt handler...
 * @param c
 */
void bser_store_char(unsigned char c, struct ring_buffer *_rx_buffer);
int bser_available(struct ring_buffer *_rx_buffer);
int bser_read(struct ring_buffer *_rx_buffer);
void bser_flush_rx(struct ring_buffer *_rx_buffer);

void dma_write(char *data, int size);
extern volatile int bser_transfered;

#endif	/* RMESERIAL_H */

