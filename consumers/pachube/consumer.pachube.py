#!/usr/bin/python
# Karl Palsson, 2010
# A karlnet consumer, publishing to pachube every so often...

import sys, os, socket
sys.path.append(os.path.join(sys.path[0], "../../common"))
import kpacket

# The keys are the node id, which is the 16bit xbee address at the moment.
config = {
    0x4201 : {
        "feedId" : 6235
        },
    0x4202 : {
        "feedId" : 6234
        },
    'apikey' : "68711d479b154637ab0f9def0f475306d73b87da19f3b15efa43ff61e25e5af9"
}

from stompy.simple import Client
import jsonpickle
import httplib
import time

import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s",
#filename="/var/log/karlnet_rrd.log")
filename="karlnet_pachube.log")
log = logging.getLogger("main")

stomp = Client(host='egri')


def runMain():
    clientid = "karlnet_pachube@%s/%d" % (socket.gethostname(), os.getpid())
    stomp.connect(clientid=clientid)
    stomp.subscribe("/topic/karlnet")

    last = time.time()
    running = {0x4201: {}, 0x4202: {}}
    
    while True:
        message = stomp.get()
        kp = jsonpickle.decode(message.body)

        # first, average them up for a little while, so we don't massively over hit the api
        if 'sensor1' in running[kp.node]:
            running[kp.node]['sensor1'] += kp.sensor1.value
        else:
            running[kp.node]['sensor1'] = kp.sensor1.value
        if 'sensor2' in running[kp.node]:
            running[kp.node]['sensor2'] += kp.sensor2.value
        else:
            running[kp.node]['sensor2'] = kp.sensor2.value
        if 'count' in running[kp.node]:
            running[kp.node]['count'] += 1
        else:
            running[kp.node]['count'] = 1
        log.info("just finished averaging for: %s", kp)
    


        if (time.time() - last > 60):
            log.info("Sending the last minutes averages...")
            headers = {"X-PachubeApiKey" : config["apikey"]}

            s1avg = running[0x4201]['sensor1'] / running[0x4201]['count']
            s2avg = running[0x4201]['sensor2'] / running[0x4201]['count']
            csv = "%d,%d,text" % (s1avg, s2avg)
            conn = httplib.HTTPConnection('www.pachube.com')
            conn.request("PUT", "/api/%d.csv" % config[0x4201]['feedId'], csv, headers) 
            log.info("uploaded 0x4201: avg1: %d, avg2: %d with response: %s", s1avg, s2avg, conn.getresponse())

            s1avg = running[0x4202]['sensor1'] / running[0x4202]['count']
            s2avg = running[0x4202]['sensor2'] / running[0x4202]['count']
            csv = "%d,%d,text" % (s1avg, s2avg)
            conn = httplib.HTTPConnection('www.pachube.com')
            conn.request("PUT", "/api/%d.csv" % config[0x4202]['feedId'], csv, headers) 
            log.info("uploaded 0x4202: avg1: %d, avg2: %d with response: %s", s1avg, s2avg, conn.getresponse())

            last = time.time()
            running = {0x4201: {}, 0x4202: {}}


if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        stomp.disconnect()
        raise SystemExit

