#!/usr/bin/python
# Karl Palsson, 2010
# A karlnet consumer, writing into a sqlite3 database

import sys, os, socket, string
sys.path.append(os.path.join(sys.path[0], "../../common"))
import kpacket

# Some basic config
config = {
    'library_file': '/home/karl/src/karlnet-git/consumers/alexandria/library.alexandria.sqlite3',
    'create_sql' : """
        create table karlnet_sensor (sampleTime text, node text, sensorType text, sensorRaw real, sensorValue real)
        """
}

from stompy.simple import Client
import jsonpickle
import rrdtool
import sqlite3

import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s"
 ,filename="/var/log/karlnet_alexandria.log")
log = logging.getLogger("main")

stomp = Client(host='egri')
try:
    conn = sqlite3.connect(config['library_file'])
except sqlite3.OperationalError as ex:
    log.error(ex)
    raise SystemExit
cur = conn.cursor()

try:
    cur.execute('select * from karlnet_sensor')
except sqlite3.OperationalError as ex:
    if string.find(str(ex), "no such table: karlnet_sensor") != -1:
        log.info("karlnet_sensor table didn't exist, creating it!")
        cur.execute(config['create_sql'])

    
# catch an exception and create the table?

def runMain():
    clientid = "karlnet_alexandria@%s/%d" % (socket.gethostname(), os.getpid())
    stomp.connect(clientid=clientid)
    stomp.subscribe("/topic/karlnet.>")

    
    while True:
        message = stomp.get()
        kp = jsonpickle.decode(message.body)
        log.info("saving into the library for: %s", kp)

        for sensor in kp.sensors:
            log.debug("saving to db for sensor: %s", sensor)
            cur.execute('insert into karlnet_sensor (sampleTime, node, sensorType, sensorRaw, sensorValue) values (?,?,?,?,?)', 
                (kp.time_received, kp.node, sensor.type, sensor.rawValue, sensor.value))
            conn.commit()
        


if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        stomp.disconnect()
        raise SystemExit

