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
from stompy.simple import Client

configFile = ConfigParser.ConfigParser()
configFile.read(['config.default.ini', 'config.ini'])

# use int(xxx, 0) because configParser.getint() doesn't magically determine hex/dec
config = {
    'node' : int(configFile.get('karlnet', 'node'), 0),
    'probe' : int(configFile.get('karlnet', 'probe'), 0),
    'apikey' : configFile.get('pachube', 'apikey'),
    'dashboardFeed' : int(configFile.get('pachube', 'dashboardFeed'),0)
}

unblob = {}

import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s",
#filename="/var/log/karlnet_pachube.log"
)
log = logging.getLogger("main")

stomp = Client(host='egri')
pygame.init()
sound = pygame.mixer.Sound("phone_2.wav")

def fetch_pachube(feedId):
    conn = httplib.HTTPConnection('www.pachube.com')
    headers = {"X-PachubeApiKey" : config["apikey"]}
    conn.request("GET", "/api/%d.json" % feedId, headers=headers)
    blob = conn.getresponse()
    if blob.status != httplib.OK:
        raise httplib.HTTPException("Couldn't read from pachube dashboard, did you set the API key?: %d %s" % (blob.status, blob.reason))
    return json.load(blob)

def get_knob(blob, tag):
    for node in blob['datastreams']:
        #log.debug("checking datastream: %s", node['tags'])
        if node['tags'][0] == tag:
            return node['values'][0]['value']
    


def runMain():
    clientid = "karlnet_pachube@%s/%d" % (socket.gethostname(), os.getpid())
    stomp.connect(clientid=clientid)
    stomp.subscribe("/topic/karlnet.>")

    last = 0
    threshold = None
    
    playing = False
    while True:
        if (time.time() - last > 60):
            log.info("Fetching current dashboard values from pachube, incase they've changed")
            unblob = fetch_pachube(feedId=config['dashboardFeed'])
            last = time.time()

        message = stomp.get()
        kp = jsonpickle.decode(message.body)
        if (kp['node'] == config['node']):
            realTemp = kp['sensors'][config['probe']]['value']
            log.info("Current temp on probe is %d", realTemp)


        threshold = get_knob(unblob, "set temp mash")
        if threshold is not None:
            setTemp = int(threshold)
            cooling = setTemp < 50
            heating = not cooling
            log.debug("threshold is %d, mode: %s", setTemp, "cooling" if cooling else "heating")
        # Assume cooling if the setTemp is under 50, assume heating if the set temp is over 50....
        if (cooling and realTemp < setTemp) or (heating and realTemp > setTemp):
            log.warn("ok, it's ready!")
            alarm = get_knob(unblob, "mash temp alarm active")
            log.debug("alarm = %s", alarm)
            if int(alarm) > 0 and not playing :
                log.info("Music is on!!!!!")
                sound.play(loops=-1)
                playing = True
            elif int(alarm) == 0 and playing:
                log.info("alarm off, and we're already playing")
                sound.stop()
                playing = False
            elif playing:
                log.debug("No need to do anything, alarm is on, and we're already playing")

        else:
            if (heating):
                log.info("Turning music off.... we're below the threshold")
            else:
                log.info("Turning music off.... we're above the threshold")
            sound.stop()
            playing = False

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
    finally:
        pygame.mixer.stop()
        pygame.mixer.quit()

