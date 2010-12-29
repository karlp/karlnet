#!/usr/bin/python
# Karl Palsson, 2010
# inject fake data frames into the ether via a serially connected xbee
# (ie, directly connected via an ftdi cable on an adafruit breakout)

__author__="karlp"

config = { 'serialPort' : "/dev/ftdi0" }
import sys, os, time
import serial
sys.path.append(os.path.join(sys.path[0], "../../common"))

from xbee import xbee, xbee_transmitter
import kpacket
#import jsonpickle
import logging
import logging.config
import logging.handlers
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s"
#,filename="/var/log/karlnet_serial.log"
)
log = logging.getLogger("main")


def runMainLoop():
    lastgoodtime = 0
    port = serial.Serial(config['serialPort'], 19200, timeout=10)
    
    while(1):
        xbtx = xbee_transmitter(port)
        fakeReadings = []
        fakeReadings.append(kpacket.Sensor(type=36, raw=0x55))
        fakeReadings.append(kpacket.Sensor(type=0xee, raw=0xff))
        fakeReadings.append(kpacket.Sensor(type=0xaa, raw=0xbb))
        data = kpacket.human_packet(node=0x6209, sensors=fakeReadings)
        #data = "cafebabe"
        log.info("injecting a fake packet into the ether...%s", data)
        xbtx.tx16(destination=0x4203, data=data.wire_format())

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

