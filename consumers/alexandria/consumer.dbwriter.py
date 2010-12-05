# Karl Palsson, Dec 2010
# Uses the Library of Alexandria to save records into a db...


import alexandria

import logging
logging.basicConfig(level=logging.DEBUG, format="%(asctime)s %(levelname)s %(name)s - %(message)s"
 ,filename="/var/log/karlnet_alexandria.log"
)
log = logging.getLogger("main")


if __name__ == "__main__":
    try:
        l = alexandria.Librarian()
        l.save_books()
    except KeyboardInterrupt:
        print "got a keyboard interrupt"
        log.info("QUIT - quitting due to keyboard interrupt")
        l.stomp.disconnect()
        raise SystemExit

