#!/usr/bin/python
# Karl Palsson, 2010
# prototyping message sending to karlnet

from stompy.simple import Client
import kpacket
import time
import logging
import jsonpickle
import random
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s")
log = logging.getLogger("main")

stomp = Client(host='egri')

def runMain():
    stomp.connect(clientid="sample producer")
    
    while True:
        s1 = kpacket.Sensor(type=36, raw=1234, value=random.randint(0,100))
        s2 = kpacket.Sensor(type=69, raw=4321, value=random.randint(40,80))
        kp = kpacket.human_packet("fake", s1, s2)
        stomp.put(jsonpickle.encode(kp), destination="/topic/karlnet")
        time.sleep(5)


if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        stomp.disconnect()
        raise SystemExit

