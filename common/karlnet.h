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

#define KPP_VERSION_1 1  // xbee-api.c, reverse endian, whichever it is...
#define KPP_VERSION_2 2  // mrf big endian? (or is that little endian?)

#define VERSION_COUNT(x,y)  (((x) << 4) | ((y) & 0x0f))

#define LM35_VREF256        35
#define TMP36_VREF256       36
#define TMP36_VREF11        37
// HCH1000 with 555, reporting humidity as a frequency
#define FREQ_1SEC           'f'
// internal temperature sensor, for devices that measure against 1.1vref
#define TEMP_INT_VREF11     'i'
// internal temperature sensor, for devices that measure against 2.56vref
#define TEMP_INT_VREF256    'I'
// 10k NTC probe, with 10k resistor divider, measured against 3.3v VCC
#define NTC_10K_3V3         'a'
// flag for binary state sensors (off/on)
#define KPS_RELAY_STATE         'b'
#define KPS_SENSOR_TEST         'z'

#endif
	
