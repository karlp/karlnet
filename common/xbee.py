# By Amit Snyderman <amit@amitsnyderman.com>
# $Id: xbee.py 3 2008-05-11 02:54:17Z amitsnyderman $
# From: http://code.google.com/p/python-xbee/
# license: MIT
# Extended and bugfixed by karlp, adding rx frames, fixing checksumming and api mode unescaping

import array
import logging

class NullHandler(logging.Handler):
    def emit(self, record):
        pass

h = NullHandler()
log = logging.getLogger("xbee")
log.addHandler(h)

def checkChecksum(data):
    """is the checksum xbee valid, plain and simple"""
    localChecksum = sum(map(ord,data))
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
    real = 0;
    escaped = 0;
    while real < length:
        curr = serial.read(1)
        if not curr:
            return None # timeout!
        if ord(curr) == xbee.ESCAPE_BYTE :
            curr = serial.read(1)
            if not curr:
                return None # timeout!
            curr = chr((ord(curr) ^ 0x20) & 0xFF)
            escaped += 1
        buf.append(curr)
        real += 1
    log.debug("read %d bytes, needed to escape %d", length, escaped)
    return buf

class xbee(object):
	
        START_IOPACKET   = 0x7e
        ESCAPE_BYTE      = 0x7D
        SERIES1_IOPACKET = 0x83
        SERIES1_RXPACKET_16 = 0x81
        MAX_PACKET_LENGTH = 100  # todo - double check this.
                
	
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
                
		if self.app_id == xbee.SERIES1_RXPACKET_16:
			addrMSB = p[1]
			addrLSB = p[2]
			self.address_16 = (addrMSB << 8) + addrLSB
			self.rssi = p[3]
			self.address_broadcast = ((p[4] >> 1) & 0x01) == 1
			self.pan_broadcast = ((p[4] >> 2) & 0x01) == 1
			self.rxdata = p[5:-1]
			self.checksum = p[-1]
                        log.info("xbee packet: %s", self)
		
		
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
					local_checksum += digMSB + digLSB
					dig = (digMSB << 8) + digLSB
					for i in range(len(dataD)):
						if dataD[i] == 0:
							dataD[i] = dig & 1
						dig = dig >> 1
				
				self.digital_samples.append(dataD)
				
				analog_count = None
				dataADC = [-1] * 6
				analog_channels = self.channel_indicator_high >> 1
				for i in range(len(dataADC)):
					if (analog_channels & 1) == 1:
						dataADCMSB = p[9 + i * n]
						dataADCLSB = p[10 + i * n]
						local_checksum += dataADCMSB + dataADCLSB
						dataADC[i] = ((dataADCMSB << 8) + dataADCLSB) / 64
						analog_count = i
					analog_channels = analog_channels >> 1
				
				self.analog_samples.append(dataADC)
				
			

	def __str__(self):
            basic = "<xbee {app_id: %#x, address_16: %#x, rssi: %s, address_broadcast: %s, pan_broadcast: %s, checksum: %d, " % (self.app_id, self.address_16, self.rssi, self.address_broadcast, self.pan_broadcast, self.checksum)
            if self.app_id == xbee.SERIES1_IOPACKET:
		return basic + ("total_samples: %s, digital: %s, analog: %s}>" % (self.total_samples,
self.digital_samples, self.analog_samples))
            if self.app_id == xbee.SERIES1_RXPACKET_16:
		return basic + ("rxdata: %s}>" % self.rxdata)
            else:
                return basic + " unknown packet type}>"
