# By Amit Snyderman <amit@amitsnyderman.com>
# $Id: xbee.py 3 2008-05-11 02:54:17Z amitsnyderman $
# From: http://code.google.com/p/python-xbee/
# license: MIT
# Extended and bugfixed by karlp, adding rx frames, fixing checksumming and api mode unescaping

import struct

import logging

class NullHandler(logging.Handler):
    def emit(self, record):
        pass

h = NullHandler()
log = logging.getLogger("xbee")
log.addHandler(h)

def checkChecksum(data):
    """is the checksum xbee valid, plain and simple"""
    localChecksum = sum(map(ord, data))
    if (localChecksum & 0xff) != 0xff:
        log.error("Checksum should have summed to 0xff, but summed to: %#x", localChecksum)
        return False
    else:
        log.debug("checksum valid!")
        return True

def readAndUnescape(serial, length):
    """
    read length _real_ bytes from serial. More will be read if any needed escaping
    This is because xbee's simply insert the escaping on the fly.  The packet length reported at the start
    of the frame does NOT include escape chars!
    """
    buf = []
    real = 0
    escaped = 0
    while real < length:
        curr = serial.read(1)
        if not curr:
            return None # timeout!
        if ord(curr) == xbee.ESCAPE_BYTE:
            curr = serial.read(1)
            if not curr:
                return None # timeout!
            curr = chr((ord(curr) ^ 0x20) & 0xFF)
            escaped += 1
        buf.append(curr)
        real += 1
    log.debug("read %d bytes, needed to escape %d", length, escaped)
    return buf

class IllegalStateException(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)

class xbee(object):
	
    START_IOPACKET   = 0x7e
    ESCAPE_BYTE      = 0x7D
    X_ON             = 0x11
    X_OFF = 0x13
    SERIES1_IOPACKET = 0x83
    SERIES1_RXPACKET_64 = 0x80
    SERIES1_RXPACKET_16 = 0x81
    SERIES1_TXPACKET_16 = 0x01
    SERIES1_MODEM_STATUS = 0x8a
    MAX_PACKET_LENGTH = 100  # todo - double check this.

    def _needs_escaping(self, byte):
        v = ord(byte)
        if ((v == xbee.START_IOPACKET) or (v == xbee.ESCAPE_BYTE) or (v == xbee.X_ON) or (v == xbee.X_OFF)):
            return True
        else:
            return False

    def escape(self, data):
        frame = ""
        for byte in data:
            if self._needs_escaping(byte):
                frame += chr(xbee.ESCAPE_BYTE)
                frame += chr((ord(byte) ^ 0x20))
            else:
                frame += byte
        return frame

    def tx16(self, destination, data, frame_id=1, disable_ack=False, broadcast=False):
        """Create a fully escaped complete frame of data for API mode 2, ready to be written to an xbee somewhere"""

        frame = struct.pack("> b", xbee.START_IOPACKET)
        frame += struct.pack("> H", len(data) + 5)
        frame += struct.pack("> b", xbee.SERIES1_TXPACKET_16)
        frame += struct.pack("> b", (frame_id & 0xff))
        frame += struct.pack("> H", destination)
        if disable_ack or broadcast:
            raise NotImplementedError("I haven't written support for these flags yet")
        else:
            frame += struct.pack("> b", 0x00)
        frame += data

        checksum = sum(map(ord, frame[3:]))
        log.debug("before truncation and substraction: %d (%#x)", checksum, checksum)
        checksum &= 0xff
        log.debug("before subtraction, %d, (%#x)", checksum, checksum)
        checksum = 0xff - checksum

        # now, we've calculated checksum, and got the entire frame ready, now go through and escape it
        log.debug("Before escaping: %s", self._pretty(frame))
        # oh yeah, but we don't escape framing and length bytes.
        escaped = frame[0:2] + self.escape(frame[2:])
        log.debug("after escaping: %s", self._pretty(escaped))

        escaped += struct.pack("> b", checksum)
        return escaped

    def _pretty(self, frame):
        line = ""
        for b in frame:
            line += "%#x " % (ord(b))
        return line

class xbee_receiver(xbee):
    def find_packet(serial):
        """
            Try to reliably find and decode one single solitary packet from the serial port
            Rejects invalid lengths.
            Call this in a loop until you get a packet back, rather than None
            """
        log.debug("Starting to look for a packet")
        char = serial.read()
        if not char:
            log.debug("SERIAL READ TIMEOUT")
            return None

        data = None
        if ord(char) == xbee.START_IOPACKET:
            # note, no guarantee that these are good packets!
            lengthMSB = ord(serial.read())
            lengthLSB = ord(serial.read())
            length = (lengthLSB + (lengthMSB << 8)) + 1
            if length > xbee.MAX_PACKET_LENGTH:
                log.warn("Dropping invalid length packet. %d > maxlength", length)
                return None
            else:
                log.debug("will attempt to read %d bytes to finish the packet", length)
                data = readAndUnescape(serial, length)
        else:
            log.debug("Found instead: %#x", ord(char))
            return None

        if checkChecksum(data):
            return data
        else:
            return None

    find_packet = staticmethod(find_packet)
	
    def __init__(self, arg):
        """
            arg is a the main packet body
                """
        self.digital_samples = []
        self.analog_samples = []
        self.init_with_packet(arg)
	
    def init_with_packet(self, p):
        p = [ord(c) for c in p]
		
        self.app_id = p[0]
        log.debug("decoding packet type: %#x. raw=%s", self.app_id, p)

        if self.app_id == xbee.SERIES1_TXPACKET_16:
            self.frame_id = p[1]
            addrMSB = p[2]
            addrLSB = p[3]
            self.address_16 = (addrMSB << 8) + addrLSB
            options = p[4]
            self.disable_ack = (options & 0x01) == 1
            self.pan_broadcast = (options & 0x04) == 1
            self.rfdata = p[5:-1]
            self.checksum = p[-1]
            log.info("xbee_tx16: %s", self)
                
        if self.app_id == xbee.SERIES1_RXPACKET_16:
            addrMSB = p[1]
            addrLSB = p[2]
            self.address_16 = (addrMSB << 8) + addrLSB
            self.rssi = p[3]
            self.address_broadcast = ((p[4] >> 1) & 0x01) == 1
            self.pan_broadcast = ((p[4] >> 2) & 0x01) == 1
            self.rfdata = p[5:-1]
            self.checksum = p[-1]
            log.info("xbee_rx16: %s", self)
		
        if self.app_id == xbee.SERIES1_RXPACKET_64:
            self.address_64 = ("%#x:%#x:%#x:%#x:%#x:%#x:%#x:%#x") % (p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8])
            self.rssi = p[9]
            self.address_broadcast = ((p[10] >> 1) & 0x01) == 1
            self.pan_broadcast = ((p[10] >> 2) & 0x01) == 1
            self.rfdata = p[11:-1]
            self.checksum = p[-1]
            log.info("xbee_rx64: %s", self)

        if self.app_id == xbee.SERIES1_MODEM_STATUS:
            self.flags = p[1]
            self.checksum = p[2]
            log.info("xbee_modem_status: %s", self)
		
        if self.app_id == xbee.SERIES1_IOPACKET:
            addrMSB = p[1]
            addrLSB = p[2]
            self.address_16 = (addrMSB << 8) + addrLSB
			
            self.rssi = p[3]
            self.address_broadcast = ((p[4] >> 1) & 0x01) == 1
            self.pan_broadcast = ((p[4] >> 2) & 0x01) == 1
			
            self.total_samples = p[5]
            self.channel_indicator_high = p[6]
            self.channel_indicator_low = p[7]
			
            for n in range(self.total_samples):
                dataD = [-1] * 9
                digital_channels = self.channel_indicator_low
                digital = 0
				
                for i in range(len(dataD)):
                    if (digital_channels & 1) == 1:
                        dataD[i] = 0
                        digital = 1
                    digital_channels = digital_channels >> 1
				
                if (self.channel_indicator_high & 1) == 1:
                    dataD[8] = 0
                    digital = 1
				
                if digital:
                    digMSB = p[8]
                    digLSB = p[9]
                    dig = (digMSB << 8) + digLSB
                    for i in range(len(dataD)):
                        if dataD[i] == 0:
                            dataD[i] = dig & 1
                        dig = dig >> 1
				
                self.digital_samples.append(dataD)
				
                dataADC = [-1] * 6
                analog_channels = self.channel_indicator_high >> 1
                for i in range(len(dataADC)):
                    if (analog_channels & 1) == 1:
                        dataADCMSB = p[9 + i * n]
                        dataADCLSB = p[10 + i * n]
                        dataADC[i] = ((dataADCMSB << 8) + dataADCLSB) / 64
                    analog_channels = analog_channels >> 1
				
                self.analog_samples.append(dataADC)
				
			

    def __str__(self):
        if self.app_id == xbee.SERIES1_IOPACKET:
            basic = "<xbee {app_id: %#x, address_16: %#x, rssi: %s, address_broadcast: %s, pan_broadcast: %s, checksum: %d, " % (self.app_id, self.address_16, self.rssi, self.address_broadcast, self.pan_broadcast, self.checksum)
            return basic + ("total_samples: %s, digital: %s, analog: %s}>" % (self.total_samples,
                            self.digital_samples, self.analog_samples))
        if self.app_id == xbee.SERIES1_RXPACKET_16:
            basic = "<xbee {app_id: %#x, address_16: %#x, rssi: %s, address_broadcast: %s, pan_broadcast: %s, checksum: %d, " % (self.app_id, self.address_16, self.rssi, self.address_broadcast, self.pan_broadcast, self.checksum)
            return basic + ("rfdata: %s}>" % self.rfdata)
        if self.app_id == xbee.SERIES1_RXPACKET_64:
            basic = "<xbee {app_id: %#x, address_64: %s, rssi: %s, address_broadcast: %s, pan_broadcast: %s, checksum: %d, " % (self.app_id, self.address_64, self.rssi, self.address_broadcast, self.pan_broadcast, self.checksum)
            return basic + ("rfdata: %s}>" % self.rfdata)
        if self.app_id == xbee.SERIES1_TXPACKET_16:
            basic = "<xbee_tx16 {app_id: %#x, address_16: %#x, frame_id=%#x, disable_ack: %s, pan_broadcast: %s, checksum: %d, rfdata=%s}>" \
                % (self.app_id, self.address_16, self.frame_id, self.disable_ack, self.pan_broadcast, self.checksum, self.rfdata)
            return basic
        if self.app_id == xbee.SERIES1_MODEM_STATUS:
            return "<xbee_modem_status {app_id=%#x, flags:%#x, checksum: %d}>" % (self.app_id, self.flags, self.checksum)
        else:
            return basic + " unknown packet type}>"
