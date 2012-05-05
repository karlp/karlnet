#!/usr/bin/python
# Karl Palsson, 2010
# A karlnet consumer, writing into the cacti and rrd.cgi rrd files...

import sys, os, socket
sys.path.append(os.path.join(sys.path[0], "../../common"))
import kpacket

# The keys are the node id, which is the 16bit xbee address at the moment.
config = {
    16897 : {
        "cgi_filename" :'/home/karlp/public_html/rrd/tinytemp1.rrd'
    }
}

import mosquitto
import jsonpickle
import rrdtool

import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s"
#,filename="/var/log/karlnet_rrd.log"
)
log = logging.getLogger("main")

clientid = "karlnet_rrd@%s/%d" % (socket.gethostname(), os.getpid())
mqttc = mosquitto.Mosquitto(clientid)

def on_message(obj, msg):
        kp = jsonpickle.decode(msg.payload)
        log.info("updating RRD for: %s", kp)
        
        if (config.get(kp.node, None)) : # slightly funky safe check
            args = "N:%f:%d" % (kp.sensors[0].value, kp.sensors[1].value)
            rrdtool.update(config[kp.node]["cgi_filename"], '--template', 'tmp36:onboard', args)
        else:
            log.info("Ignoring rrd update for non-configured node: %s", kp.node)


def runMain():
    mqttc.connect("localhost")
    mqttc.on_message = on_message
    mqttc.subscribe("karlnet/readings/#")
    
    while True:
        mqttc.loop()


if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        mqttc.disconnect()
        raise SystemExit

