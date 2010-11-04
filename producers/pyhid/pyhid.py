#!/usr/bin/python
# Karl Palsson, May 2010
# pyusb based hid listener for a pjrc.com teensy board.
# the teensy is just dumping xbee packet data out the usb bus.

__author__="karlp"

# Not actually used yet, as I just made these the defaults anyway...
config = {
    'hiddevice': {
# Matches defines in usb_debug_only.c from pjrc
        'vendor': 0x16c0,
        'product': 0x0479
    }
}

import sys, os, time
sys.path.append(os.path.join(sys.path[0], "../../common"))

from xbee import xbee
import kpacket
from stompy.simple import Client
import jsonpickle
from threading import Thread
import Queue

import usb.core
import usb.util

import logging
import logging.config
import logging.handlers
logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s - %(message)s"
# comment out the line below for testing :)
,filename="/var/log/karlnet_serial.log"
)
log = logging.getLogger("main")


class Usbreader(Thread):
  """
  Class that reads in from a teensy usb, and stuffs all the data into a queue, for someone else to look at
  """

  def __init__(self, queue):
    Thread.__init__(self)
    self.data = queue
    self.log = logging.getLogger("Usbreader")

  def openPort(self, vendor=0x16c0, product=0x0479, detachKernel=True):
    """
    Attempt to open a HID device with the given vendor and product ids
    unless requested not to, any kernel driver will be detached
    """
    teensy = usb.core.find(idVendor=vendor, idProduct=product)
    if teensy is None:
        self.log.info('no matching hid board found found: %s, %s', vendor, product)
        return None

    # more magic because we know all about this particular HID device
    cfg = teensy[0]
    intf = cfg[(0,0)]
    ep = intf[0]

    if ep is None:
        self.log.warn("Valid hid device, but invalid endpoint?")
        return None

    if (detachKernel):
        try :
            teensy.detach_kernel_driver(0)
        except usb.core.USBError as e:
            # already detached...
            pass

    return (teensy, intf, ep)


  def run(self):

    new_data = []
    lastgoodtime = 0
    manualTimeout = 40
    blob = None
    teensy = {}
    intf = {}
    ep = {}

    while 1:
        if time.time() - lastgoodtime > manualTimeout:
            blob = self.openPort()
            if blob is None:
                self.log.info("Couldn't open the device, will try again shortly....")
                time.sleep(2)
                continue
            (teensy, intf, ep) = blob
        try :
            # at most 32 bytes, 6000ms timeout
            new_data = teensy.read(ep.bEndpointAddress, 32, intf.bInterfaceNumber, 6000);
        except usb.core.USBError as e:
            self.log.error("oops: %s", e)  # oh well, try and open it again in a bit...
            lastgoodtime = 0
            # this lets us survive through plug/unplug but not device reset for some reason
            usb.util.dispose_resources(teensy)
            time.sleep(2)
            continue
	
        lastgoodtime = time.time()
        self.log.debug("usb data is %d items long: %s", len(new_data), new_data)
        
        for x in new_data:
            self.data.put(chr(x))
        

#### END OF Usbreader class

class FakeSerial():
    """
    Wraps a queue to make it sort of look  like a serial port
    """
    
    def __init__(self, new_queue):
        self.queue = new_queue    

    def read(self, length=1):
        try:
            return self.queue.get(True, 1)
        except Queue.Empty:
            return None
    

def runMainLoop():
    data_queue = Queue.Queue()
    teensy = Usbreader(data_queue)
    teensy.daemon = True
    teensy.start()

    serial = FakeSerial(data_queue)

    stomp = Client(host="egri")
    stomp.connect(clientid="teensy usb listener", username="karlnet", password="password")

    while 1:
        packet = xbee.find_packet(serial)
        if packet:
            xb = xbee(packet)
        else:
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
        hp = kpacket.human_packet(node=xb.address_16, sensors=kp.sensors)
        hp.time_received = time.time()
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


