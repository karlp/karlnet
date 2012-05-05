#!/usr/bin/python
# Karl Palsson, 2010
# The library of alexandria, providing a Librarian that writes karlnet data into a datbase
# and also a Researcher() that provides methods for getting data back out again

import sys, os, socket, string
sys.path.append(os.path.join(sys.path[0], "../../common"))
import sqlite3
import mosquitto
import jsonpickle
import kpacket
import logging
import unittest

class NullHandler(logging.Handler):
    def emit(self, record):
        pass

h = NullHandler()
logging.getLogger("alexandria").addHandler(h)

# Some shared config
config = {
    'library_file': '/home/karlp/src/karlnet-git/consumers/alexandria/library.alexandria.sqlite3',
    'create_sql' : """
        create table karlnet_sensor (sampleTime integer, node integer, sensorType integer, channel integer, sensorRaw
real, sensorValue real)""",
    'create_index' : """
        create index idx_karlnet_sensor_sampleTime on karlnet_sensor(sampleTime);
        """,
    'mq_host' : 'egri'
}

class Researcher:

    def __init__(self, file=None):
        self.log = logging.getLogger("alexandria.Researcher")
        if file is None:
            file = config['library_file']
        try:
            self.conn = sqlite3.connect(file)
        except sqlite3.OperationalError as ex:
            self.log.error(ex)
            raise ex  # reraise for the world to handle...

    def last_values(self, node, sensorType=None, count=100, since=None):
        """
    Look up the last count values in the library for the given node, and optional type
    returns a list of tuples for each sensor type detected
    return data is like... map = [{node:1, type:t2, data:[[][][][]]}, {node:2...]
        """
        c = self.conn.cursor()
        # Using limit <count> actually isn't very suitable, 
        # if the node isn't found, it will scan the entire table, should use
        # count, plus the sample interval, to get something like, x last seconds worth
        if sensorType is None and since is None:
            self.log.debug("getting allll types")
            c.execute("""
            select sampleTime, sensorType, channel, sensorValue from karlnet_sensor
            where node = ?
            order by sampleTime desc limit ?
            """, (node,count,))
        elif sensorType is None and since is not None:
            # all in the last x
            c.execute("""select sampleTime, sensorType, channel, sensorValue from karlnet_sensor
            where node = ? and sampleTime > ? order by sampleTime desc""", (node, since,))
        elif since is None:
            # sensorType must be defined, but not since
            c.execute("""select sampleTime, sensorType, channel, sensorValue
                from karlnet_sensor where node = ? and sensorType = ?
                order by sampleTime desc limit ?""", (node, sensorType, count,))
        else:
            # sensor type defined, and also since
            c.execute("""select sampleTime, sensorType, channel, sensorValue
                from karlnet_sensor where node = ? and sensorType = ? and sampleTime > ?
                order by sampleTime desc""", (node, sensorType, since,))

        map = []
        data = {}
        for row in c:
            sampleTime = round(row[0] * 1000)
            type = row[1]
            channel = row[2]
            value = row[3]
            self.log.debug("pushing sampletime %d, value %f into type %s/%s", sampleTime, value, type, channel)
            key = (type, channel)
            if (key not in data):
                data[key] = []
            data[key].append([sampleTime, value])

        for key in data:
            self.log.debug("muxing in %s", key)
            (type, channel) = (key)
            data[key].reverse()
            map.append({'node': node, 'type':type, 'channel' : channel, 'data': data[key]})
        return map
    
class Librarian:

    def __init__(self, file=None, host=None):
        """
Make a database writing librarian, listening to packets from host, and writing into the the sqlite3 file specified.
Defaults provided match my own personal environment.
If the database file is not found, or does not contain the appropriate tables and indexes, it is created
        """
        self.log = logging.getLogger("alexandria.Librarian")
        if file is None:
            file = config['library_file']
        try:
            self.conn = sqlite3.connect(file)
        except sqlite3.OperationalError as ex:
            self.log.error(ex)
            raise ex

        self.log.debug("library exists, validating schema")
        self.cur = self.conn.cursor()
        try:
            self.cur.execute('select * from karlnet_sensor')
        except sqlite3.OperationalError as ex:
            if string.find(str(ex), "no such table: karlnet_sensor") != -1:
                self.log.debug("karlnet_sensor table didn't exist, creating it!")
                self.cur.execute(config['create_sql'])
                self.cur.execute(config['create_index'])
        self.log.debug("library is valid, connecting to mq")

        if host is None:
            host = config['mq_host']
        # FIXME - get clientid here...
        clientid = "karlnet_alexandria@%s/%d" % (socket.gethostname(), os.getpid())
        self.mqttc = mosquitto.Mosquitto(clientid, obj=self)
        self.log.info("Valid library and messaging is up, starting to write books!")

    def on_message(self, obj, msg):
            kp = jsonpickle.decode(msg.payload)
            self.log.info("saving into the library for: %s", kp)

            for i in range(len(kp.sensors)):
                sensor = kp.sensors[i]
                if sensor.type == ord('z'):
                    self.log.debug("Skipping archiving of sensor test value: %s", sensor)
                    return
                self.log.debug("saving to db for sensor %d: %s", i, sensor)
                self.cur.execute('insert into karlnet_sensor (sampleTime, node, sensorType, channel, sensorRaw, sensorValue) values (?,?,?,?,?,?)', 
                    (kp.time_received, kp.node, sensor.type, i, sensor.rawValue, sensor.value))
                self.conn.commit()

    def save_books(self):
        """
Loop forever, saving readings into the database.  TODO: topic could be a config option?
        """
        self.mqttc.connect()
        self.mqttc.subscribe("karlnet/readings/#")
        self.mqttc.on_message = self.on_message

        while True:
            self.mqttc.loop()

# Keeping this as it shows me how to do it, without looking it up again
class TestResearcher(unittest.TestCase):
    def test_splitKey(self):
        self.assertEqual("0", "0")

if __name__ == '__main__':
    unittest.main()
