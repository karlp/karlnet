#!/usr/bin/python
# Karl Palsson, 2010
# inject fake data frames into the ether via a serially connected xbee
# (ie, directly connected via an ftdi cable on an adafruit breakout)

__author__="karlp"

config = { 'serialPort' : "/dev/ftdi0" }
import sys
import os
import time
import serial
import optparse
import struct

sys.path.append(os.path.join(sys.path[0], "../../common"))

from xbee import xbee
import kpacket
#import jsonpickle
import logging
import logging.config
import logging.handlers
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s"
#,filename="/var/log/karlnet_serial.log"
)
log = logging.getLogger("main")

parser = optparse.OptionParser()
parser.add_option("-p", "--port", dest="port",
    help="Serial port to use [default: %default]", default=config['serialPort'])
parser.add_option("-m", "--manual", dest="manual", action="store_true",
    help="Inject packets one at a time, on keyboard input [default=%default]", default=True)
parser.add_option("-a", "--auto", dest="manual", action="store_false",
    help="Inject packets one per second, no user intervention required (opposite of manual)")

(options, args) = parser.parse_args()



def runMainLoop():
    lastgoodtime = 0
    port = serial.Serial(config['serialPort'], 19200, timeout=10)
    
    while(1):
        xbtx = xbee()
        fakeReadings = []
        fakeReadings.append(kpacket.Sensor(type=36, raw=0x55))
        fakeReadings.append(kpacket.Sensor(type=0xee, raw=0xff))
        fakeReadings.append(kpacket.Sensor(type=0xaa, raw=0x12345678))
        data = kpacket.human_packet(node=0x6209, sensors=fakeReadings)
        #data = "cafebabe"
        #data = struct.pack("> 5s", "abcde")
        if options.manual:
            log.debug("Press enter to send the packet")
            sys.stdin.readline()
            log.info("injecting a fake packet into the ether...%s", data)
            wiredata = xbtx.tx16(destination=0x6001, data=data.wire_format())
            #wiredata = xbtx.tx16(destination=0x6001, data=data)
            port.write(wiredata)
        else:
            log.info("injecting a fake packet into the ether...%s", data)
            wiredata = xbtx.tx16(destination=0x6001, data=data.wire_format())
            port.write(wiredata)
            time.sleep(1)



if __name__ == "__main__":
    try:
        runMainLoop()
    except KeyboardInterrupt:
        print " ok, quitting :) " 
        log.info("QUIT - Quitting due to keyboard interrupt :)")
        raise SystemExit
    except:
        log.exception("Bang! something went wrong :(")

