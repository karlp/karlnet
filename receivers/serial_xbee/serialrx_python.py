# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="karl"
__date__ ="$Sep 30, 2009 10:56:02 PM$"

filename = '/var/lib/cacti/rra/localhost_freq_9.rrd'

import serial
import subprocess
import logging
# Log everything, and send it to stderr.
logging.basicConfig(level=logging.DEBUG)

def convertTempSensor(rawValue):
    logging.debug("raw = *%s*", rawValue);
    rawNum = float(rawValue.strip().strip('\0'))
    milliVolts = rawNum / 1023 * 5000;
    lessOffset = (milliVolts - 750) / 10;
    tempC = 25 + lessOffset;
    return tempC


def runMainLoop():
    #port = serial.Serial("COM3", 19200, timeout=3)
    port = serial.Serial("/dev/ttyUSB0", 19200, timeout=2)
    freq = 0
    temp = 0
    while (1) :
          for sensor in [1, 2]:
            raw = port.readline()
            logging.debug("raw line: *%s*", raw)
            sensorId = raw[:1]
            sensorValue = raw[2:]
            if sensorId == "1":
                temp = convertTempSensor(sensorValue)
            elif sensorId == "2":
                freq = int(sensorValue) # chop newline...
            else:
                logging.warn("no sensor value for %d ", sensor)

          args = "N:%f:%d" % (temp, freq)
          #ret = subprocess.call(['rrdtool', 'update', 'target.rrd', args])
          ret = subprocess.call(['rrdtool', 'update', filename, args])

          logging.info("temp = %f, freq = %d", temp, freq)

if __name__ == "__main__":
    try:
        runMainLoop()
    except KeyboardInterrupt:
        logging.info(" ok, quitting :)")
        raise SystemExit

