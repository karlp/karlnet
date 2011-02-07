// Karl Palsson, 2010
// packet format for my sensors.

// header is just for simple sanity.
// note, no node id, we'll use the radio nodes for that.
#ifndef __KPACKET_H
#define __KPACKET_H
#include <stdint.h>

// max max, but you should only declar as many as you need.
#define MAX_SENSORS     4

// A single sensor reading.
typedef struct _ksensor {
    uint8_t     type;
    uint32_t    value;
} ksensor;

// a collection of sensor readings
typedef struct _kpacket {
	uint8_t 	header;
        uint8_t         version:4;
        uint8_t         nsensors:4;
        ksensor         ksensors[MAX_SENSORS];
} kpacket;

// a collection of sensor readings
typedef struct _kpacket2 {
        uint8_t 	header;
        uint8_t         versionCount;
        ksensor         ksensors[MAX_SENSORS];
} kpacket2;

#define VERSION_COUNT(x,y)  (((x) << 4) | ((y) & 0x0f))

#endif
	
