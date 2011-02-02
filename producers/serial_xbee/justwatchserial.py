#!/usr/bin/python
# Karl Palsson, 2010
# Listen to a serial port connected to a xbee/karlnet, and post all the data to stomp

__author__="karlp"

#config = { 'serialPort' : "/dev/ftdi0" }
config = { 'serialPort' : "/dev/ttyACM0" }
import sys, os, time
import serial
from optparse import OptionParser # argparse looks nice, but I only have py2.6
sys.path.append(os.path.join(sys.path[0], "../../common"))

from xbee import xbee, xbee_receiver
import kpacket
from stompy.simple import Client
import jsonpickle
import logging
import logging.config
import logging.handlers

parser = OptionParser()
parser.add_option("-t", "--test", dest="testmode", action="store_true",
                  help="Run in test mode (don't post any reports to stomp, log to console)", default=False)
parser.add_option("-p", "--port", dest="port", help="Serial port to use [default: %default]", default=config['serialPort'])

(options, args) = parser.parse_args()

if options.testmode:
    stomp = None
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s - %(message)s")
else:
    stomp = Client(host='egri')
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s - %(message)s",filename="/var/log/karlnet_serial.log")

log = logging.getLogger("main")

def runMainLoop():
    lastgoodtime = 0
    port = None
    manualTimeout = 40

    if stomp:
        stomp.connect(clientid="serial port listener", username="karlnet", password="password")

    while(1):
        if time.time() - lastgoodtime > manualTimeout:
            log.warn("XXX Reopening the serial port, no data for %d seconds!", manualTimeout)
            if port:
                port.close()
            port = serial.Serial(options.port, 19200, timeout=10)
	packet = xbee_receiver.find_packet(port)
        if packet:
                xb = xbee_receiver(packet)
	else:
                log.warn("NO PACKET FOUND")
		continue
	
	try:
            if xb.app_id == xbee.SERIES1_RXPACKET_16:
                kp = kpacket.wire_packet(xb.rfdata)
            else:
                log.warn("Received a packet, but not a normal rx, was instead: %#x", xb.app_id)
                continue
	except kpacket.BadPacketException as e:
		log.warn("Couldn't decode: %s" % e.msg)
		continue
        lastgoodtime = time.time()
        hp = kpacket.human_packet(node=xb.address_16, sensors=kp.sensors)
        if stomp:
            stomp.put(jsonpickle.encode(hp), destination = "/topic/karlnet.%d" % hp.node)
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

