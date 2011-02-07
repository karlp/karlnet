#!/usr/bin/python
# Karl Palsson, 2011
# Listen to a node dumping raw kpackets over a serial pipe
# Virtually identical to justwatchserial.py, only the scanning for 
# the packet is different

__author__="karlp"

config = { 'serialPort' : "/dev/ttyACM1" }
import sys, os, time
import serial
from optparse import OptionParser # argparse looks nice, but I only have py2.6
sys.path.append(os.path.join(sys.path[0], "../../common"))

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
    logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s")
else:
    stomp = Client(host='egri')
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s - %(message)s",filename="/var/log/karlnet_serial_local.log")

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

        wireRx = kpacket.wire_receiver()
        raw = wireRx.find_raw_packet(port)
        if not raw:
                log.warn("NO PACKET FOUND")
		continue

        try:
            kp = kpacket.wire_packet(raw)
	except kpacket.BadPacketException as e:
		log.warn("Couldn't decode: %s" % e.msg)
		continue
        lastgoodtime = time.time()
        hp = kpacket.human_packet(node=0x0001, sensors=kp.sensors)
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

