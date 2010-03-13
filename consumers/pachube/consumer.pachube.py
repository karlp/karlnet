#!/usr/bin/python
# Karl Palsson, 2010
# A karlnet consumer, publishing to pachube every so often...

# I'm not real happy with how this keeps the api key in the source. But I've never really dealt with api keys before, so I'm not sure of a better
# way... at least, not just yet.

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


def upload(node, running):
    """Average a set of data and upload to pachube. Assumes that a config exists for the node id."""
    s1avg = running['sensor1'] / running['count']
    s2avg = running['sensor2'] / running['count']
    csv = "%d,%d,text" % (s1avg, s2avg)
    conn = httplib.HTTPConnection('www.pachube.com')
    headers = {"X-PachubeApiKey" : config["apikey"]}
    conn.request("PUT", "/api/%d.csv" % config[node]['feedId'], csv, headers) 
    log.info("uploaded node:%#x: avg1: %d, avg2: %d with response: %s", node, s1avg, s2avg, conn.getresponse())


def runMain():
    clientid = "karlnet_pachube@%s/%d" % (socket.gethostname(), os.getpid())
    stomp.connect(clientid=clientid)
    stomp.subscribe("/topic/karlnet")

    last = time.time()
    running = {}
    
    while True:
        message = stomp.get()
        kp = jsonpickle.decode(message.body)

        # first, average them up for a little while, so we don't massively over hit the api
        nd = running.setdefault(kp.node, {})
        nd['sensor1'] = nd.setdefault('sensor1', 0) + kp.sensor1.value
        nd['sensor2'] = nd.setdefault('sensor2', 0) + kp.sensor2.value
        nd['count'] = nd.setdefault('count', 0) + 1
        log.info("just finished averaging for: %s", kp)

        if (time.time() - last > 60):
            log.info("Sending the last minutes averages...")

            for key in running:
                if config.get(key) :
                    log.info("Averaging stats for node: %#x for upload to pachube", key)
                    upload(key, running[key])
                else:
                    log.warn("No pachube config for node: %#x, perhaps you should make some?", key)
            
            last = time.time()
            running = {}


if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        stomp.disconnect()
        raise SystemExit
    except:
        log.exception("Something really bad!")
        stomp.disconnect()
        raise
