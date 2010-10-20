#!/usr/bin/python
# Karl Palsson, 2010
# A karlnet consumer, writing into the cacti and rrd.cgi rrd files...

import sys, os, socket
sys.path.append(os.path.join(sys.path[0], "../../common"))
import kpacket

# The keys are the node id, which is the 16bit xbee address at the moment.
config = {
    16897 : {
        "cacti_filename" : '/var/lib/cacti/rra/localhost_freq_9.rrd',
        "cgi_filename" :'/home/karl/public_html/rrd/tinytemp1.rrd'
    }
}

from stompy.simple import Client
import jsonpickle
import rrdtool

import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s",
filename="/var/log/karlnet_rrd.log")
log = logging.getLogger("main")

stomp = Client(host='egri')

def runMain():
    clientid = "karlnet_rrd@%s/%d" % (socket.gethostname(), os.getpid())
    stomp.connect(clientid=clientid)
    stomp.subscribe("/topic/karlnet.>")
    
    while True:
        message = stomp.get()
        kp = jsonpickle.decode(message.body)
        log.info("updating RRD for: %s", kp)
        
        if (config.get(kp.node, None)) : # slightly funky safe check
            args = "N:%f:%d" % (kp.sensors[0].value, kp.sensors[1].value)
            rrdtool.update(config[kp.node]["cacti_filename"], '--template', 'temp:freq', args)
            rrdtool.update(config[kp.node]["cgi_filename"], '--template', 'tmp36:onboard', args)
        else:
            log.info("Ignoring rrd update for non-configured node: %s", kp.node)


if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        stomp.disconnect()
        raise SystemExit

