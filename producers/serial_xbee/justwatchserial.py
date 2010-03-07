#!/usr/bin/python
# Karl Palsson, 2010
# Listen to a serial port connected to a xbee/karlnet, and post all the data to stomp

__author__="karlp"

config = { 'serialPort' : "/dev/ttyUSB0" }

import serial
from xbee import xbee
import sys
sys.path.append("../../common")
import kpacket
import time
from stompy.simple import Client
import jsonpickle
import logging
import logging.config
import logging.handlers
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s",
filename="/var/log/karlnet_serial.log")
log = logging.getLogger("main")

stomp = Client(host='egri')


def runMainLoop():
    lastgoodtime = 0
    port = None
    manualTimeout = 40
    
    stomp.connect(clientid="serial port listener")

    while(1):
        if time.time() - lastgoodtime > manualTimeout:
            log.warn("XXX Reopening the serial port, no data for %d seconds!", manualTimeout)
            if port:
                port.close()
            port = serial.Serial(config['serialPort'], 19200, timeout=10)
	packet = xbee.find_packet(port)
        if packet:
                xb = xbee(packet, strict=True)
	else:
                log.warn("NO PACKET FOUND")
		continue
	
	try:
            if xb.app_id == xbee.SERIES1_RXPACKET_16:
                kp = kpacket.wire_packet(xb.rxdata)
            else:
                log.warn("Received a packet, but not a normal rx, was instead: %#x", xb.app_id)
                continue
	except kpacket.BadPacketException as e:
		log.warn("Couldn't decode: %s" % e.msg)
		continue
        lastgoodtime = time.time()
        hp = kpacket.human_packet(node=xb.address_16, sensor1=kp.sensor1, sensor2=kp.sensor2)
        stomp.put(jsonpickle.encode(hp), destination = "/topic/karlnet")
        log.info(hp)


if __name__ == "__main__":
    try:
        runMainLoop()
    except KeyboardInterrupt:
        print " ok, quitting :) " 
        log.info("QUIT - Quitting due to keyboard interrupt :)")
        raise SystemExit

