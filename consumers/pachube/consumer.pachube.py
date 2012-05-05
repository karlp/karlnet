#!/usr/bin/python
# Karl Palsson, 2010
# A karlnet consumer, publishing to pachube every so often...

# api key here is limited to only uploading from my home computer,
# you'll definitely need your own

import sys, os, socket
sys.path.append(os.path.join(sys.path[0], "../../common"))
import kpacket
from optparse import OptionParser
import mosquitto
import jsonpickle
import httplib
import time

import logging

# The keys are the node id, which is the 16bit xbee address at the moment.
config = {
    0x4201 : {
        "feedId" : 6235
        },
    0x4202 : {
        "feedId" : 6234
        },
    0x0001 : {
        "feedId" : 6447
        },
    0x6002 : {
        "feedId" : 25684
    },
    'apikey' : "cmG5jxylcl9geEUPe1psgko-aGs8bEXhLpOaHNmeXEE"
}

parser = OptionParser()
parser.add_option("-t", "--test", dest="testmode", action="store_true",
    help="Run in test mode (don't post any data anywhere, log to console)",
    default=False)

(options, args) = parser.parse_args()

if options.testmode:
    logging.basicConfig(level=logging.DEBUG,
        format="%(asctime)s %(levelname)s %(name)s - %(message)s")
else:
    logging.basicConfig(level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s - %(message)s",
        filename="/var/log/karlnet_pachube.log")

log = logging.getLogger("main")


def upload(node, running):
    """Average a set of data and upload to pachube. Assumes that a config exists for the node id."""
    sensorStrings = []
    for sensor in running['sensors']:
        log.debug("sensor is: %d, count is %d", running['sensors'][sensor], running['count'])
        avg = running['sensors'][sensor] / (1.0 * running['count'])
        sensorStrings.append("%3.2f" % avg)

    csv = ','.join(sensorStrings)
    log.debug("Derived averages, trying to upload: %s", csv)
    conn = httplib.HTTPConnection('www.pachube.com')
    headers = {"X-PachubeApiKey" : config["apikey"]}
    url = "/api/%d.csv" % config[node]['feedId']
    if options.testmode:
        log.info("TEST: Would have uploaded %s to: %s with headers: %s", csv, url, headers)
    else:
        conn.request("PUT", url, csv, headers)
        response = conn.getresponse()
        log.info("uploaded node:%#x: csv:: %s with response: %d - %s",
            node, csv, response.status, response.reason)

class Application:
    running = {}
    last = time.time() 

    def on_message(self, obj, msg):
        kp = jsonpickle.decode(msg.payload)

        # first, average them up for a little while, so we don't massively over hit the api
        nd = self.running.setdefault(kp.node, {})
        nd.setdefault('sensors', {})
        for i in range(len(kp.sensors)):
            nd['sensors'][i] = nd['sensors'].setdefault(i, 0) + kp.sensors[i].value
            log.debug("Averaged sensor: %s", kp.sensors[i])

        nd['count'] = nd.setdefault('count', 0) + 1
        log.info("just finished averaging for: %s", kp)

        if (time.time() - self.last > 60):
            log.info("Sending the last minutes averages...")

            for key in self.running:
                if config.get(key) :
                    log.info("Averaging stats for node: %#x for upload to pachube", key)
                    upload(key, self.running[key])
                else:
                    log.warn("No pachube config for node: %#x, perhaps you should make some?", key)
            
            self.last = time.time()
            self.running = {}



    def runMain(self):
        clientid = "karlnet_pachube@%s/%d" % (socket.gethostname(), os.getpid())
        mqttc = mosquitto.Mosquitto(clientid, obj=self)
        mqttc.connect("localhost")
        mqttc.subscribe("karlnet/readings/#")
        mqttc.on_message = self.on_message

        while True:
            mqttc.loop()


if __name__ == "__main__":
    try:
        a = Application()
        a.runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        raise SystemExit
    except:
        log.exception("Something really bad!")
        raise

