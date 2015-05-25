#!/usr/bin/env python3
"""
data.sparkfun.com outputter, should of course work with any "phant" server
"""
import argparse
import json
import time
import requests
import paho.mqtt.client as paho

def on_connect(client, userdata, rc):
    client.subscribe("karlnet/readings/4369")

def on_message(client, userdata, msg):
    assert(msg.topic.split("/")[2] == "4369")

    if time.time() - userdata.last_sent > 60:
        mm = json.loads(msg.payload.decode("utf-8"))
        headers = {
            'Content-type': 'application/x-www-form-urlencoded',
            'Phant-Private-Key': userdata.private_key,
        }

        data = {
            "temp": mm["sensors"][0]["value"],
            "humidity" : mm["sensors"][1]["value"],
        }
        if len(mm["sensors"]) > 2:
            data["channel1_temp"] = mm["sensors"][2]["value"]
        else:
            data["channel1_temp"] = 0

        print("posting data: ", data)
        resp = requests.post("http://data.sparkfun.com/input/%s" % userdata.public_key, data=data, headers=headers)
        resp.raise_for_status()
        userdata.last_sent = time.time()
        print("got a response: ", resp)


def get_parser():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-H", "--host", help="mqtt host", default="localhost")
    parser.add_argument("-k", "--private_key", help="data stream private key", required=True)
    parser.add_argument("-p", "--public_key", help="data stream public key", required=True)
    return parser

def main(opts):
    opts.last_sent = 0
    mqttc = paho.Client(userdata=opts)
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message
    mqttc.connect(opts.host)

    should_run = True
    while should_run:
        try:
            mqttc.loop()
        except KeyboardInterrupt:
            should_run = False
            print("Ok, exiting on ctrl-c")
        except Exception:
            should_run = False
            print("Unhandled exception!")
            raise


    mqttc.disconnect()

if __name__ == "__main__":
    options = get_parser().parse_args()
    main(options)

