// Karl Palsson, 2011
//
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "uart.h"

#define BAUD 57600

static FILE mystdout = {0};
static FILE mystdout1 = {0};

static int _uart_putc(char c, FILE* stream) {
    if (c == '\n') {
        _uart_putc('\r', NULL);
    }
    uart_putc(c);
    return 0;
}
static int _uart1_putc(char c, FILE* stream) {
    if (c == '\n') {
        _uart1_putc('\r', NULL);
    }
    uart1_putc(c);
    return 0;
}

static void init(void) {
    clock_prescale_set(clock_div_1);
    uart_init(UART_BAUD_SELECT(BAUD, F_CPU));
    uart1_init(UART_BAUD_SELECT(BAUD, F_CPU));
    fdev_setup_stream(&mystdout, _uart_putc, NULL, _FDEV_SETUP_WRITE);
    fdev_setup_stream(&mystdout1, _uart1_putc, NULL, _FDEV_SETUP_WRITE);
    stdout = &mystdout;
    DDRD |= _BV(PIND6);
    DDRE |= _BV(PINE3);
}

int main(void) {
    init();
    sei();
    uint32_t i = 0;
    while (1) {
        PORTD |= (1 << PIND6);
        PORTE &= ~(1 << PINE3);
        _delay_ms(500);
        printf_P(PSTR("hello karl, this is uart0, loop %d\n"), i++);
        fprintf_P(&mystdout1, PSTR("this is uart1, loop %d\n"), i);
        PORTD &= ~(1 << PIND6);
        PORTE |= (1 << PINE3);
        _delay_ms(250);
    }
}

