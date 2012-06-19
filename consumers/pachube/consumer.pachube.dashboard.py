#!/usr/bin/python
# Karl Palsson, 2011
# A karlnet consumer, that also pulls from a pachube dashboard to take actions.
# An alternative to having to set up triggers and expose a url to let them call

# I'm not real happy with how this keeps the api key in the source. But I've never really dealt with api keys before, so I'm not sure of a better
# way... at least, not just yet.

# this might be better than pygame? 
# but is very linux specific:
# http://www.kryogenix.org/days/2010/12/21/playing-system-sounds-from-python-programs-on-ubuntu

import os
import time
import socket
import ConfigParser

import json
import jsonpickle
import httplib

import pygame
import mosquitto

configFile = ConfigParser.ConfigParser()
configFile.read(['config.default.ini', 'config.ini'])

# use int(xxx, 0) because configParser.getint() doesn't magically determine hex/dec
config = {
    'node' : int(configFile.get('karlnet', 'node'), 0),
    'probe' : int(configFile.get('karlnet', 'probe'), 0),
    'apikey' : configFile.get('pachube', 'apikey'),
    'dashboardFeed' : int(configFile.get('pachube', 'dashboardFeed'),0)
}

class Container():
    unblob = {}

import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s",
#filename="/var/log/karlnet_pachube.log"
)
log = logging.getLogger("main")

pygame.init()
sound = pygame.mixer.Sound("phone_2.wav")

qq = Container()

def fetch_pachube(feedId):
    conn = httplib.HTTPConnection('api.cosm.com')
    headers = {"X-APIKey" : config["apikey"]}
    conn.request("GET", "/v1/feeds/%d.json" % feedId, headers=headers)
    blob = conn.getresponse()
    if blob.status != httplib.OK:
        raise httplib.HTTPException("Couldn't read from pachube dashboard, did you set the API key?: %d %s" % (blob.status, blob.reason))
    return json.load(blob)

def get_knob(blob, tag):
    for node in blob['datastreams']:
        #log.debug("checking datastream: %s", node['tags'])
        if node['tags'][0] == tag:
            return node['values'][0]['value']
    
def handle_mq_packet(obj, msg):
        kp = jsonpickle.decode(msg.payload)
        if (kp['node'] == config['node']):
            realTemp = kp['sensors'][config['probe']]['value']
            log.info("Current temp on probe is %d", realTemp)
        else:
            return

        threshold = get_knob(obj.unblob, "set temp mash")
        if threshold is not None:
            setTemp = int(threshold)
            cooling = setTemp < 50
            heating = not cooling
            log.debug("threshold is %d, mode: %s", setTemp, "cooling" if cooling else "heating")
        # Assume cooling if the setTemp is under 50, assume heating if the set temp is over 50....
        if (cooling and realTemp < setTemp) or (heating and realTemp > setTemp):
            log.warn("ok, it's ready!")
            alarm = get_knob(obj.unblob, "mash temp alarm active")
            log.debug("alarm = %s", alarm)
            if int(alarm) > 0 and not obj.playing :
                log.info("Music is on!!!!!")
                sound.play(loops=-1)
                obj.playing = True
            elif int(alarm) == 0 and obj.playing:
                log.info("alarm off, and we're already playing")
                sound.stop()
                obj.playing = False
            elif obj.playing:
                log.debug("No need to do anything, alarm is on, and we're already playing")

        else:
            if (heating):
                log.info("Turning music off.... we're below the threshold")
            else:
                log.info("Turning music off.... we're above the threshold")
            sound.stop()
            obj.playing = False

def runMain(blob):
    clientid = "karlnet_pachube@%s/%d" % (socket.gethostname(), os.getpid())
    mqttc = mosquitto.Mosquitto(clientid, obj=blob)
    mqttc.connect("localhost")
    mqttc.subscribe("karlnet/readings/#")
    mqttc.on_message = handle_mq_packet

    last = 0
    threshold = None
    
    blob.playing = False
    while True:
        if (time.time() - last > 60):
            log.info("Fetching current dashboard values from pachube, incase they've changed")
            blob.unblob = fetch_pachube(feedId=config['dashboardFeed'])
            last = time.time()

        mqttc.loop()


if __name__ == "__main__":
    try:
        runMain(qq)
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        raise SystemExit
    except:
        log.exception("Something really bad!")
        raise
    finally:
        pygame.mixer.stop()
        pygame.mixer.quit()

