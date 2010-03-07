// Karl Palsson, 2010
#ifndef __XBEE_API_H
#define __XBEE_API_H

#include "karlnet.h"
#include <stdint.h>


void xbee_send_16(uint16_t destination, kpacket packet);

#endif
