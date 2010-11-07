#!/usr/bin/python
# Karl Palsson, 2010
# Listen to a serial port connected to a xbee/karlnet, and dump to console

__author__="karlp"

config = { 'serialPort' : "/dev/ftdi0" }
import sys, os, time
import serial
sys.path.append(os.path.join(sys.path[0], "../../common"))

from xbee import xbee
import kpacket
import jsonpickle
import logging
import logging.config
import logging.handlers
logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s - %(message)s"
#,filename="/var/log/karlnet_serial.log"
)
log = logging.getLogger("main")


def runMainLoop():
    lastgoodtime = 0
    port = None
    manualTimeout = 40
    
    while(1):
        if time.time() - lastgoodtime > manualTimeout:
            log.warn("XXX Reopening the serial port, no data for %d seconds!", manualTimeout)
            if port:
                port.close()
            port = serial.Serial(config['serialPort'], 19200, timeout=10)
	packet = xbee.find_packet(port)
        if packet:
                xb = xbee(packet)
	else:
                log.warn("NO PACKET FOUND")
		continue
	
	try:
            if xb.app_id == xbee.SERIES1_RXPACKET_16:
                kp = kpacket.wire_packet(xb.rfdata)
            elif xb.app_id == xbee.SERIES1_TXPACKET_16:
                kp = kpacket.wire_packet(xb.rfdata)
            else:
                log.warn("Received a packet, but not a normal rx, was instead: %#x", xb.app_id)
                continue
	except kpacket.BadPacketException as e:
		log.warn("Couldn't decode: %s" % e.msg)
		continue
        lastgoodtime = time.time()
        hp = kpacket.human_packet(node=xb.address_16, sensors=kp.sensors)
        log.info(hp)


if __name__ == "__main__":
    try:
        runMainLoop()
    except KeyboardInterrupt:
        print " ok, quitting :) " 
        log.info("QUIT - Quitting due to keyboard interrupt :)")
        raise SystemExit
    except:
        log.exception("Bang! something went wrong :(")

