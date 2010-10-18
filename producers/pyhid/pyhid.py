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

import usb.core
import usb.util

import logging
import logging.config
import logging.handlers
logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(name)s - %(message)s"
# comment out the line below for testing :)
#,filename="/var/log/karlnet_serial.log"
)
log = logging.getLogger("main")



def openPort(vendor=0x16c0, product=0x0479, detachKernel=True):
    """
    Attempt to open a HID device with the given vendor and product ids
    unless requested not to, any kernel driver will be detached
    """
    teensy = usb.core.find(idVendor=vendor, idProduct=product)
    if teensy is None:
        log.info('no matching hid board found found')
        return None

    # more magic because we know all about this particular HID device
    cfg = teensy[0]
    intf = cfg[(0,0)]
    ep = intf[0]

    if ep is None:
        log.warn("Valid hid device, but invalid endpoint?")
        return None

    if (detachKernel):
        try :
            teensy.detach_kernel_driver(0)
        except usb.core.USBError as e:
            # already detached...
            pass

    return (teensy, intf, ep)


def runMainLoop():

    data = []
    lastgoodtime = 0
    manualTimeout = 40
    blob = None
    teensy = {}
    intf = {}
    ep = {}

    while 1:
        if time.time() - lastgoodtime > manualTimeout:
            blob = openPort()
            if blob is None:
                log.info("Couldn't open the device, will try again shortly....")
                time.sleep(2)
                continue
            (teensy, intf, ep) = blob
        try :
            # at most 32 bytes, 6000ms timeout
            data = teensy.read(ep.bEndpointAddress, 32, intf.bInterfaceNumber, 6000);
        except usb.core.USBError as e:
            log.error("oops: %s", e)  # oh well, try and open it again in a bit...
            lastgoodtime = 0
            # this lets us survive through plug/unplug but not device reset for some reason
            usb.util.dispose_resources(teensy)
            time.sleep(2)
            continue
	
        lastgoodtime = time.time()
        sret = ''.join([chr(x) for x in data])
        print sret


if __name__ == "__main__":
    try:
        runMainLoop()
    except KeyboardInterrupt:
        print " ok, quitting :) "
        log.info("QUIT - Quitting due to keyboard interrupt :)")
        raise SystemExit
    except:
        log.exception("Bang! something went wrong :(")


