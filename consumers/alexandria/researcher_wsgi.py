#!/usr/bin/python
# Change working directory so relative paths (and template lookup) work again
import os
# this came from the bottle samples, but doesn't work/do anything?
#os.chdir(os.path.dirname(__file__))
import sys

import bottle
import json
# ... add or import your bottle app code here ...
from bottle import route, run, request, response, abort

sys.path.append("/home/karl/src/karlnet-git/common")
sys.path.append("/home/karl/src/karlnet-git/consumers")
from alexandria.alexandria import Researcher

@route('/hello')
def hello():
    return "Hello World!"

@route('/data/:node')
def node_data(node):
    r = Researcher()
    since = request.GET.get("since")
    count = request.GET.get("count")
    ## FIXME - arguabaly, the / 1000 should be done client side...?
    ## well, seeing as the researcher returns with * 1000, then no, the / 1000 should be here too
    ## look, make sure you're consistent with it is all I'm saying :)
    if since is None and count is None:
        blob = r.last_values(node)
    elif since is None and count is not None:
        blob = r.last_values(node, count=count)
    elif since is not None and count is None:
        blob = r.last_values(node, since=(float(since)/1000))
    else:
        # perhaps, since, but at most count?
        abort(500,"count as well as since? what do you want me to do here?")

    return fix_blob(blob)

@route('/data/:node/:type')
def node_type_data(node, type):
    r = Researcher()
    since = request.GET.get("since")
    count = request.GET.get("count")
    if since is None and count is None:
        blob = r.last_values(node, sensorType=type)
    elif since is None and count is not None:
        blob = r.last_values(node, sensorType=type, count=count)
    elif since is not None:
        blob = r.last_values(node, sensorType=type, since=(float(since)/1000))
    else:
        raise NotImplementedError("count as well as since? what do you want me to do here?")

    return fix_blob(blob)

def fix_blob(blob):
    callback = request.GET.get("callback")
    if callback is None:
        return json.dumps(blob)
    else:
        response.content_type = "application/javascript"
        return ("%s(%s);" % (callback, json.dumps(blob)))
        

# Do NOT use bottle.run() with mod_wsgi
application = bottle.default_app()



