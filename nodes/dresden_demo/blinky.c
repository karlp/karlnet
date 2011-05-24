// Karl Palsson, 2011
// 
#include <avr/io.h> 
#include <util/delay.h>

void init() {
    DDRF = (1<<PINF2);
    DDRE = (1<<PINE3);
    DDRD = (1<<PIND6);
}


int main(void) {
    init();
    while (1) {
        PORTF |= (1<<PINF2);
        PORTE |= (1<<PINE3);
        PORTD |= (1<<PIND6);
        _delay_ms(500);
        PORTF &= ~(1<<PINF2);
        PORTE &= ~(1<<PINE3);
        PORTD &= ~(1<<PIND6);
        _delay_ms(250);
    }
}

