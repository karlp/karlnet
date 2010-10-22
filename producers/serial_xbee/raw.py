#!/usr/bin/python
# Karl Palsson, 2010
# Listen to a serial port connected to a xbee/karlnet, just log it all to the screen

__author__="karlp"

config = { 'serialPort' : "/dev/ftdi0" }
import sys, os, time
import serial
sys.path.append(os.path.join(sys.path[0], "../../common"))

from xbee import xbee
import kpacket
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
    
        lastgoodtime = time.time()


if __name__ == "__main__":
    try:
        runMainLoop()
    except KeyboardInterrupt:
        print " ok, quitting :) " 
        log.info("QUIT - Quitting due to keyboard interrupt :)")
        raise SystemExit
    except:
        log.exception("Bang! something went wrong :(")

