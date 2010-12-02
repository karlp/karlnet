#!/usr/bin/python
# Karl Palsson, 2010
# The library of alexandria, providing a Librarian that writes karlnet data into a datbase
# and also a Researcher() that provides methods for getting data back out again

import sys, os, socket, string
sys.path.append(os.path.join(sys.path[0], "../../common"))
import sqlite3
from stompy.simple import Client
import jsonpickle
import kpacket
import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s"
# ,filename="/var/log/karlnet_alexandria.log"
)
log = logging.getLogger("main")

# Some shared config
config = {
    #'library_file': '/home/karl/src/karlnet-git/consumers/alexandria/library.alexandria.sqlite3',
    'library_file': 'demo.sqlite3',
    'create_sql' : """
        create table karlnet_sensor2 (sampleTime integer, node integer, sensorType integer, sensorRaw
real, sensorValue real)""",
    'create_index' : """
        create index idx_karlnet_sensor2_sampleTime on karlnet_sensor2(sampleTime);
        """,
    'stomp_host' : 'egri'
}

class Researcher:

    def __init__(self, file=None):
        if file is None:
            file = config['library_file']
        try:
            self.conn = sqlite3.connect(file)
        except sqlite3.OperationalError as ex:
            log.error(ex)
            raise ex  # reraise for the world to handle...

    def last_values(self, node, sensorType=None, count=100):
        """
    Look up the last count values in the library for the given node, and optional type
    returns a list of tuples for each sensor type detected
    return data is like... map = [{node:1, type:t2, data:[[][][][]]}, {node:2...]
        """
        c = self.conn.cursor()
        if sensorType is None:
            log.debug("getting allll types")
            c.execute("""
            select sampleTime, sensorType, sensorValue from karlnet_sensor2
            where node = ?
            order by sampleTime desc limit ?
            """, (node,count,))
        else:
            c.execute("""select sampleTime, sensorType, sensorValue
                from karlnet_sensor2 where node = ? and sensorType = ?
                order by sampleTime desc limit ?""", (node, sensorType, count,))

        map = []
        types = {}
        for row in c:
            log.debug("pushing sampletime %s, value %s into type %s", row[0], row[2], row[1])
            if (row[1] not in types):
                types[row[1]] = []
            types[row[1]].append([row[0], row[2]])

        for type in types:
            log.debug("muxing in %s", type)
            map.append({'node': node, 'type':type, 'data': types[type]})
        return map
    
class Librarian:

    def __init__(self, file=None, host=None):
        if file is None:
            file = config['library_file']
        try:
            self.conn = sqlite3.connect(file)
        except sqlite3.OperationalError as ex:
            log.error(ex)
            raise ex

        log.debug("library exists, validating schema")
        self.cur = self.conn.cursor()
        try:
            self.cur.execute('select * from karlnet_sensor2')
        except sqlite3.OperationalError as ex:
            if string.find(str(ex), "no such table: karlnet_sensor2") != -1:
                log.debug("karlnet_sensor2 table didn't exist, creating it!")
                self.cur.execute(config['create_sql'])
                self.cur.execute(config['create_index'])
        log.debug("library is valid, connecting to stomp")

        if host is None:
            host = config['stomp_host']
        self.stomp = Client(host=host)
        log.info("Valid library and stomp is up, starting to write books!")

    def save_books(self):
        clientid = "karlnet_alexandria@%s/%d" % (socket.gethostname(), os.getpid())
        self.stomp.connect(clientid=clientid)
        self.stomp.subscribe("/topic/karlnet.>")
    
        while True:
            message = self.stomp.get()
            kp = jsonpickle.decode(message.body)
            log.info("saving into the library for: %s", kp)

            for sensor in kp.sensors:
                log.debug("saving to db for sensor: %s", sensor)
                self.cur.execute('insert into karlnet_sensor2 (sampleTime, node, sensorType, sensorRaw, sensorValue) values (?,?,?,?,?)', 
                    (kp.time_received, kp.node, sensor.type, sensor.rawValue, sensor.value))
                self.conn.commit()


if __name__ == "__main__":
    try:
        l = Librarian()
        l.save_books()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        l.stomp.disconnect()
        raise SystemExit

