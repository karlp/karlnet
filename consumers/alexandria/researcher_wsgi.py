#!/usr/bin/python
# Change working directory so relative paths (and template lookup) work again
import os
# this came from the bottle samples, but doesn't work/do anything?
#os.chdir(os.path.dirname(__file__))
import sys

import bottle
import json
# ... add or import your bottle app code here ...
from bottle import route, run, request, response

sys.path.append("/home/karl/src/karlnet-git/common")
sys.path.append("/home/karl/src/karlnet-git/consumers")
from alexandria.alexandria import Researcher

@route('/hello')
def hello():
    return "Hello World!"

@route('/data/:node')
def node_data(node):
    r = Researcher()
    blob = {
        'label' : node,
        'data' : [[0,1], [1,2], [2,4], [3,2], [4,1], [5,0]]
    }
    blob = r.last_values(node)
    callback = request.GET.get("callback")
    if callback is None:
        return json.dumps(blob)
    else:
        response.content_type = "application/javascript"
        return ("%s(%s);" % (callback, json.dumps(blob)))
        

# Do NOT use bottle.run() with mod_wsgi
application = bottle.default_app()



