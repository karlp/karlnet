#/usr/bin/python
# Karl Palsson, 2010
# listener for my karlnet sensors

import time
import sys
import stomp
import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s")
log = logging.getLogger("main")


class MyListener(object):
    def on_error(self, headers, message):
        log.error("Something went wrong! %s", message)

    def on_message(self, headers, message):
        log.info("received a message: %s", message)


conn = stomp.Connection(host_and_ports=[('egri', 61613)], prefer_localhost=False)
conn.set_listener('listenernode1', MyListener())

def runMain():
    conn.start()
    conn.connect()
    conn.subscribe(destination='/topic/karlnet')

if __name__ == "__main__":
    try:
        runMain()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        conn.stop()
        raise SystemExit
