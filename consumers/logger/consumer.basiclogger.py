#!/usr/bin/python
# Karl Palsson, 2010
# A basic karlnet consumer, just logging what's going by.

from stompy.simple import Client
import jsonpickle
import sys, os
sys.path.append(os.path.join(sys.path[0], "../../common"))

import kpacket
import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s")
log = logging.getLogger("main")

stomp = Client(host='egri')

def runMain():
    stomp.connect()
    stomp.subscribe("/topic/karlnet")
    
    while True:
        message = stomp.get()
        
        log.info("received a message: %s", message.body)
        kp = jsonpickle.decode(message.body)
        log.info("received a message: %s", kp)


if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        stomp.disconnect()
        raise SystemExit

