// Karl Palsson
// hacky tx only xbee api mode....
// YOU ARE EXPECTED TO DEFINE PUT_CHAR youself!

#include "xbee-api.h"

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
		PUT_CHAR(ESCAPE);
		PUT_CHAR(value ^ 0x20);
	} else {
		PUT_CHAR(value);
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
	PUT_CHAR(FRAME_DELIM);
	PUT_CHAR(0);  // we never send more than 255 bytes
        // 5 = command, frame, destination + options, 2 = kpacket header
	PUT_CHAR(5 + 2 + (packet.nsensors * sizeof(ksensor)));

	uint8_t checksum;  // doesn't need to include escaping! woo

	checksum = escapePutChar(0x01);  // tx 16 command id
	checksum += escapePutChar(1);  // frame id, not used, but better to allow acks than explicitly disable them
	checksum += escapePut16(destination);
	checksum += escapePutChar(0); // options
	checksum += escapePutChar(packet.header);
	checksum += escapePutChar((packet.version << 4) | (packet.nsensors & 0x0f));
	//checksum += escapePutChar(packet.nsensors);
        for (int i = 0; i < packet.nsensors; i++) {
	    checksum += escapePutChar(packet.ksensors[i].type);
	    checksum += escapePut32(packet.ksensors[i].value);
        }
	
	PUT_CHAR(0xff - checksum);
}
