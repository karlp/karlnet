#!/usr/bin/python
# Karl Palsson, 2010
# A karlnet consumer, publishing to pachube every so often...

import sys, os, socket
sys.path.append(os.path.join(sys.path[0], "../../common"))
import kpacket

# The keys are the node id, which is the 16bit xbee address at the moment.
config = {
    0x4202 : {
        "feedId" : 6234
        },
    'apikey' : "68711d479b154637ab0f9def0f475306d73b87da19f3b15efa43ff61e25e5af9"
}

from stompy.simple import Client
import jsonpickle
import httplib

import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s",
#filename="/var/log/karlnet_rrd.log")
filename="karlnet_pachube.log")
log = logging.getLogger("main")

stomp = Client(host='egri')


def buildEeml(kp):
    """Construct EEML from a karlnet packet"""
    data = """<?xml version="1.0" encoding="UTF-8"?>
<eeml xmlns="http://www.eeml.org/xsd/005">
  <environment>
    <data id="0">
        <value>%(value1)s</value>
    </data>
  </environment>
</eeml>"""
    return data % {'value1' : kp.sensor1.value}

def runMain():
    clientid = "karlnet_pachube@%s/%d" % (socket.gethostname(), os.getpid())
    stomp.connect(clientid=clientid)
    stomp.subscribe("/topic/karlnet")
    
    while True:
        message = stomp.get()
        kp = jsonpickle.decode(message.body)
        log.info("updating pachube for: %s", kp)

        if (kp.node == 0x4202) :
            log.info("so far so good...")
            eeml = buildEeml(kp)
            print eeml
            log.info("using node: %#x and feedid %d", kp.node, config[kp.node]['feedId'])
            conn = httplib.HTTPConnection('www.pachube.com')
            headers = {"X-PachubeApiKey" : config["apikey"]}
            #conn.request("PUT", "/api/%d.eeml" % config[kp.node]['feedId'], eeml, headers) 
            csv = "%d,%d,text" % (kp.sensor1.value, kp.sensor2.value)
            conn.request("PUT", "/api/%d.csv" % config[kp.node]['feedId'], csv, headers) 
            print conn.getresponse()
            
            
            


if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        stomp.disconnect()
        raise SystemExit

