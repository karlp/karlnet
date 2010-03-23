# Karl Palsson, 2010

import logging

class human_packet(object):
    """
    Defines the human post processed packet. Something that client applications would use
    """
    def __init__(self, node, sensors):
        """Make a human packet from some basic data
        
        :param node: The nodeid this sensor data is from.
        :param sensors: A list of sensor objects
        """
        self.node = node
        self.sensors = sensors

    def __str__(self):
        return "human_packet[node=%#x, sensors=%s]" % (self.node, self.sensors)

class wire_packet(object):
	"""
	Defines the packet format used in my sensor network, and handles decoding it from an xbee frame
	"""
        log = logging.getLogger("WirePacket")
	def __init__(self, arg):
		if arg[0] != ord('x'):
			raise BadPacketException("this isn't a kpacket: %s" % arg[0])
                if len(arg) == 11:
                    # Handle original packet format here?!!!
                    self.log.warn("Handling legacy packet format!")
                    self.sensors = []
                    self.sensors.append(Sensor(binary=arg[1:6]))
                    self.sensors.append(Sensor(binary=arg[6:11]))
                    return

                version_samples = arg[1];
                version = version_samples >> 4
                if version != 1:
                    raise BadPacketException("currently only know how to handle version 1 packets")
                num_samples = version_samples & 0xf
                if len(arg) != 2 + (num_samples * 5):
                    raise BadPacketException("this is not the right length for a kpacket! %d" % len(arg))
                self.log.debug("handling packet version: %d with %d samples", version, num_samples)
                self.sensors = []
                for i in range (0, num_samples):
                    self.sensors.append(Sensor(binary=arg[2+(i*5):7+(i*5)]))

class Sensor(object):
    """
    Defines an individual sensor reading, made up of a type, and a raw value
    """
    log = logging.getLogger("Sensors")
    def __init__(self, binary=None, type=None, raw=None, value=None):
        """If binary, then decode binary as raw sensor bytes.
        Otherwise, just make an object like we're told"""
        if binary:
            self.__decodeBinary(binary)
        else:
            self.type = type
            self.rawValue = raw
            self.value = value

    def __decodeBinary(self, arg):
        """Internal method for decoding raw binary data from a wire_packet (for instance)"""
        if len(arg) != 5:
            raise BadPacketException("sensor readings should be 5 bytes! 5!=%d" % len(arg))
        self.log.debug("Decoding bytes into a sensor object: *%s*", arg)
        self.type = arg[0]
        self.rawValue = (arg[1] << 24) + (arg[2] << 16) + (arg[3] << 8) + arg[4]
        (self.value,self.units) = self.__decode()

    
    def __convertSensor_TMP36(self, rawValue, reference):
        rawNum = float(rawValue)
        milliVolts = rawNum / 1024 * reference;
        lessOffset = (milliVolts - 750) / 10;
        tempC = 25 + lessOffset;
        return tempC
        
    def __decode(self):
        """
        decode the raw sensor value
    type is the sensor type id, known values are...
     chr(36) for tmp-36 vs a 2.56 vref
     chr(37) for tmp-36 vs a 1.1 vref
     'i' for tiny85 internal temp sensor reading, raw  (measures against 1.1vref)
     'I' for mega32u4 internal temp sensor (measures against 2.56vref)
     'f' for raw frequency measurements from HCH1000 humidity sensor
        """
        self.log.debug("Decoding type:%s, raw=%s", self.type, self.rawValue)
        if self.type == 36:
            return (self.__convertSensor_TMP36(self.rawValue, 2560), 'degreesCelsius')
        if self.type == 37:
            return (self.__convertSensor_TMP36(self.rawValue, 1100), 'degreesCelsius')
        if self.type == ord('f'):
            # 555 timer is C = 1/f / 300k / 0.693 (555 constant)
            return ((1e12/(self.rawValue * 300000 * 0.693)), 'picoFarads')
        # need calibration!
        if self.type == ord('i'):
            return (self.rawValue, 'unknown')
        if self.type == ord('I'):
            return (self.rawValue, 'unknown')
        self.log.warn("Unknown sensor type: %s", self.type)
        return (0, "na")


    def __repr__(self):
        return "sensor[t:%d, raw=%d, val=%f]" % (self.type, self.rawValue, self.value)
        

class BadPacketException(Exception):
	"""thrown when you attempt to create a kpacket from an illegal rx frame"""
	def __init__(self, msg):
	        self.msg = msg
