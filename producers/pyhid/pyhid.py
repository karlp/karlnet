# pyusb based hid listener for a pjrc.com teensy board.
# the teensy is just dumping xbee packet data out the usb bus.
# Karl Palsson, May 2010

import usb.core

# Matches defines in usb_debug_only.c from pjrc
teensy = usb.core.find(idVendor=0x16c0, idProduct=0x0479)
if teensy is None:
	raise ValueError('no teensy board found')

# more magic because we know all about this particular HID device

cfg = teensy[0]
intf = cfg[(0,0)]
ep = intf[0]

if ep is None:
	raise ValueError('couldn''t get the endpoint :(')

while 1:
	# at most 32 bytes, 6000ms timeout
	data = []
	try :
		data = teensy.read(ep.bEndpointAddress, 32, intf.bInterfaceNumber, 6000);
	except usb.core.USBError as e:
		print("oops, got", e)
	
	sret = ''.join([chr(x) for x in data])
	print sret




