/* 
 * File:   bser.c
 * Author: karlp
 * 
 * Created on June 6, 2012, 3:26 PM
 */

#include "syscfg.h"
#include "bser.h"
#include <string.h>

void bser_begin(struct ring_buffer *_rx_buffer) {
    memset(_rx_buffer, 0, sizeof (struct ring_buffer));
}

void bser_store_char(unsigned char c, struct ring_buffer *_rx_buffer) {
    int i = (unsigned int) (_rx_buffer->head + 1) % SERIAL_BUFFER_SIZE;

    // if we should be storing the received character into the location
    // just before the tail (meaning that the head would advance to the
    // current location of the tail), we're about to overflow the buffer
    // and so we don't write the character or advance the head.
    if (i != _rx_buffer->tail) {
        _rx_buffer->buffer[_rx_buffer->head] = c;
        _rx_buffer->head = i;
    }
}

int bser_available(struct ring_buffer *_rx_buffer) {
    return (unsigned int) (SERIAL_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail) % SERIAL_BUFFER_SIZE;
}

int bser_read(struct ring_buffer *_rx_buffer) {
    // if the head isn't ahead of the tail, we don't have any characters
    if (_rx_buffer->head == _rx_buffer->tail) {
        return -1;
    } else {
        unsigned char c = _rx_buffer->buffer[_rx_buffer->tail];
        _rx_buffer->tail = (unsigned int) (_rx_buffer->tail + 1) % SERIAL_BUFFER_SIZE;
        return c;
    }
}

void bser_flush_rx(struct ring_buffer *_rx_buffer) {
    // don't reverse this or there may be problems if the RX interrupt
    // occurs after reading the value of rx_buffer_head but before writing
    // the value to rx_buffer_tail; the previous value of rx_buffer_head
    // may be written to rx_buffer_tail, making it appear as if the buffer
    // don't reverse this or there may be problems if the RX interrupt
    // occurs after reading the value of rx_buffer_head but before writing
    // the value to rx_buffer_tail; the previous value of rx_buffer_head
    // may be written to rx_buffer_tail, making it appear as if the buffer
    // were full, not empty.
    _rx_buffer->head = _rx_buffer->tail;
}


