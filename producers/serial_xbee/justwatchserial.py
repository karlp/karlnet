#!/usr/bin/python
# Karl Palsson, 2010
# Listen to a serial port connected to a xbee/karlnet, and post all the data to stomp


config = {'serialPort': "/dev/ftdi0"}
import sys
import time

from optparse import OptionParser
import os
import serial
sys.path.append(os.path.join(sys.path[0], "../../common"))

from xbee import xbee, xbee_receiver
import kpacket
import mosquitto
import jsonpickle
import logging
import logging.config
import logging.handlers

parser = OptionParser()
parser.add_option("-t", "--test", dest="testmode", action="store_true",
    help="Run in test mode (don't post any reports to message broker, log to console)",
    default=False)
parser.add_option("-p", "--port", dest="port",
    help="Serial port to use [default: %default]", default=config['serialPort'])

(options, args) = parser.parse_args()

if options.testmode:
    mqttc = None
    logging.basicConfig(level=logging.DEBUG,
        format="%(asctime)s %(levelname)s %(name)s - %(message)s")
else:
    mqttc = mosquitto.Mosquitto("serial port listener")
    logging.basicConfig(level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s - %(message)s",
        filename="/var/log/karlnet/serial_xbee.log")

log = logging.getLogger("main")

def runMainLoop():
    lastgoodtime = 0
    port = None
    manualTimeout = 40

    if mqttc:
        mqttc.connect("localhost")

    while mqttc.loop() == 0:
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
                lastgoodtime = time.time()
                kp = kpacket.wire_packet(xb.rfdata)
            else:
                log.warn("Received a packet, but not a normal rx, was instead: %#x", xb.app_id)
                qq = kpacket.wire_packet(xb.rfdata)
                qqhp = kpacket.human_packet(node=xb.address_16, sensors=qq.sensors)
                log.info("was actually: %s", qqhp)
                lastgoodtime = time.time()
                continue
        except kpacket.BadPacketException as e:
            log.warn("Couldn't decode: %s" % e.msg)
            continue
        lastgoodtime = time.time()
        hp = kpacket.human_packet(node=xb.address_16, sensors=kp.sensors)
        hp.time_received = time.time()
        if mqttc:
            mqttc.publish("karlnet/readings/%d" % hp.node, jsonpickle.encode(hp))
            #stomp.put(jsonpickle.encode(hp), destination="/topic/karlnet.%d" % hp.node)
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

