# To change this template, choose Tools | Templates
# and open the template in the editor.

__author__="karl"
__date__ ="$Sep 30, 2009 10:56:02 PM$"

import serial
import subprocess

def convertTempSensor(rawValue):
    milliVolts = float(rawValue) / 1023 * 5000;
    lessOffset = (milliVolts - 750) / 10;
    tempC = 25 + lessOffset;
    return tempC


def runMainLoop():
    #port = serial.Serial("COM3", 19200, timeout=3)
    port = serial.Serial("/dev/ttyUSB0", 19200, timeout=3)
    temp = 0
    freq = 0
    for sensor in [1, 2]:
        raw = port.readline()
        sensorId = raw[:1]
        sensorValue = raw[2:]
        if sensorId == "1":
            temp = convertTempSensor(sensorValue)
        elif sensorId == "2":
            freq = int(sensorValue) # chop newline...
        else:
            print "no sensor value for ", sensor

    print ("temp:%f freq:%d" % (temp, freq))

if __name__ == "__main__":
    try:
        runMainLoop()
    except KeyboardInterrupt:
        print " ok, quitting :) " 
        raise SystemExit

