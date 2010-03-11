// Karl Palsson, 2010
#ifndef __XBEE_API_H
#define __XBEE_API_H

#include "karlnet.h"
#include <stdint.h>

#if __SUART
#define PUT_CHAR(x) putChar(x)
#endif
#if __AVR_UART
#define PUT_CHAR(x) uart_putc(x)
#endif

#ifndef PUT_CHAR
#error PUT_CHAR(x) needs to be defined 
#endif


void xbee_send_16(uint16_t destination, kpacket packet);

#endif
