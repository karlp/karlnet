// Karl Palsson, 2010
// packet format for my sensors.

// header is just for simple sanity.
// note, no node id, we'll use the radio nodes for that.
#ifndef __KPACKET_H
#define __KPACKET_H
#include <stdint.h>
typedef struct _kpacket {
	uint8_t 	header;
	uint8_t 	type1;
	uint32_t 	value1;
	uint8_t 	type2;
	uint32_t 	value2;
} kpacket;
#endif
	
