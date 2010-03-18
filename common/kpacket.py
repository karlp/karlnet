# Karl Palsson, 2010

import logging

class human_packet(object):
    """
    Defines the human post processed packet. Something that client applications would use
    """
    def __init__(self, node, sensor1, sensor2):
        """Make a human packet from some basic data
        
        :param node: The nodeid this sensor data is from.
        :param sensor1: Sensor object for sensor1
        :param sensor2: Sensor object for sensor2
        """
        self.node = node
        self.sensor1 = sensor1
        self.sensor2 = sensor2

    def __str__(self):
        return "human_packet[node=%#x, sensor1=%s, sensor2=%s]" % (self.node, self.sensor1, self.sensor2)

class wire_packet(object):
	"""
	Defines the packet format used in my sensor network, and handles decoding it from an xbee frame
	"""
	def __init__(self, arg):
		if arg[0] != ord('x'):
			raise BadPacketException("this isn't a kpacket: %s" % arg[0])
		if len(arg) != 11:
			raise BadPacketException("this is not the right length for a kpacket! %d" % len(arg))
                self.sensor1 = Sensor(binary=arg[1:6])
                self.sensor2 = Sensor(binary=arg[6:11])

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
     'i' for tiny85 internal temp sensor reading, raw
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
        if self.type == ord('i'):
            return (self.rawValue, 'unknown')
        self.log.warn("Unknown sensor type: %s", self.type)
        return (0, "na")


    def __str__(self):
        return "sensor[t:%d, raw=%d, val=%f]" % (self.type, self.rawValue, self.value)
        

class BadPacketException(Exception):
	"""thrown when you attempt to create a kpacket from an illegal rx frame"""
	def __init__(self, msg):
	        self.msg = msg
