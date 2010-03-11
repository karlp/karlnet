// Karl Palsson
// hacky tx only xbee api mode....


#include "xbee-api.h"
#include "suart.h"

#define FRAME_DELIM 0x7e
#define ESCAPE 0x7d
#define X_ON 0x11
#define X_OFF 0x13

// private methods...
int needsEscaping(uint8_t val) {
	if ((val == FRAME_DELIM) || (val == ESCAPE) || ( val == X_ON) | (val == X_OFF)) {
		return 1;
	} else {
		return 0;
	}
}

// write out value, inserting and escaping if needed.  returns the input to add in checksumming
uint8_t escapePutChar(uint8_t value) {
	if (needsEscaping(value)) {
		putChar(ESCAPE);
		putChar(value ^ 0x20);
	} else {
		putChar(value);
	}
	return value;
}

// write a 16bit value, escaping if needed, returns the sum of input bytes to add in checksumming
uint8_t escapePut16(uint16_t value) {
	return escapePutChar(value >> 8) + escapePutChar(value & 0xff);
}

uint8_t escapePut32(uint32_t value) {
	return escapePut16(value >> 16) + escapePut16(value & 0xffff);
}


// send a karlpacket.  Not exactly very general :)
void xbee_send_16(uint16_t destination, kpacket packet) {
	putChar(FRAME_DELIM);
	putChar(0);  // we never send more than 255 bytes
	putChar(5 + sizeof(kpacket));  // command, frame, destination + options

	uint8_t checksum;  // doesn't need to include escaping! woo

	checksum = escapePutChar(0x01);  // tx 16 command id
	checksum += escapePutChar(1);  // frame id, not used, but better to allow acks than explicitly disable them
	checksum += escapePut16(destination);
	checksum += escapePutChar(0); // options
	checksum += escapePutChar(packet.header);
	checksum += escapePutChar(packet.type1);
	checksum += escapePut32(packet.value1);
	checksum += escapePutChar(packet.type2);
	checksum += escapePut32(packet.value2);
	
	putChar(0xff - checksum);
}
