#!/usr/bin/python
# Karl Palsson, 2011
# A karlnet consumer, that also pulls from a pachube dashboard to take actions.
# An alternative to having to set up triggers and expose a url to let them call

# I'm not real happy with how this keeps the api key in the source. But I've never really dealt with api keys before, so I'm not sure of a better
# way... at least, not just yet.

import os
import time
import socket

import json
import jsonpickle
import httplib
from stompy.simple import Client

# Which node and sensor number do we want to compare against the pachube dashboard
config = {
    'node': 0x4201,
    'probe': 2,
    'apikey' : "68711d479b154637ab0f9def0f475306d73b87da19f3b15efa43ff61e25e5af9"
}

import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s",
#filename="/var/log/karlnet_pachube.log"
)
log = logging.getLogger("main")

stomp = Client(host='egri')

def fetch_pachube(feedId, tag):
    conn = httplib.HTTPConnection('www.pachube.com')
    headers = {"X-PachubeApiKey" : config["apikey"]}
    conn.request("GET", "/api/%d.json" % feedId, headers=headers)
    blob = conn.getresponse()
    unblob = json.load(blob)
    for node in unblob['datastreams']:
        log.debug("checking datastream: %s", node['tags'])
        if node['tags'][0] == tag:
            return node['values'][0]['value']
    


def runMain():
    clientid = "karlnet_pachube@%s/%d" % (socket.gethostname(), os.getpid())
    stomp.connect(clientid=clientid)
    stomp.subscribe("/topic/karlnet.>")

    last = 0
    threshold = None
    
    while True:
        if (time.time() - last > 60):
            log.debug("Fetching current dashboard values from pachube, incase they've changed")
            threshold = fetch_pachube(feedId=12484, tag="set temp mash")
            log.debug("threshold = %s", threshold)
            last = time.time()

        message = stomp.get()
        kp = jsonpickle.decode(message.body)
        if (kp['node'] == config['node']):
            realTemp = kp['sensors'][config['probe']]['value']
            log.info("Current temp on probe is %d", realTemp)


        if threshold is not None and realTemp > threshold:
            # TODO - play a sound here or something....
            log.warn("ok, it's ready!")


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

