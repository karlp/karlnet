// simple graphing stomp received values, one line per node, only sensor 0 is used
// 

$(function () {

    if (!window.WebSocket) {
        $("#placeholder").replaceWith("Websockets not supported by this browser");
        return;
    }

    var config = {
        nodes : {
            0x4203 : "teensyhumi",
            0x4201 : "tinytemp",
            0xbabe : "dummy producer",
            0x0001 : "receiver station",
            0x6002 : "fridge controller"
        },
        sensors : {
            35 : "lm35 (&deg;C)",
            36 : "tmp36 (&deg;C)",
            37 : "tmp36b (&deg;C)",
            73 : "internalTemp (raw)",
            98 : "motor state",
            102 : "humidity (pF)",
            105 : "internalTemp (raw)"
        },
        yaxis : {
            35 : 1,
            36 : 1,
            37 : 1,
            73 : 1,
            98 : 2,
            102 : 2,
            105 : 1
        }
    };

    var options = {
        lines: {
            show: true
        },
        points: {
            show: true
        },
        xaxis: {
            mode : 'time'
        },
        grid: {
            hoverable: true
        },
        legend : {
            position: "nw"
        }
    };
    var data = {};

    function make_key(type, channel) {
        return "" + type + "_" + channel;
    }

    // directly from flot examples...
    function showTooltip(x, y, contents) {
        $('<div id="tooltip" class="tooltip">' + contents + '</div>').css( {
            top: y + 5,
            left: x + 5
        }).appendTo("body").fadeIn(200);
    }

    function makeTooltip(event, pos, item) {
        if (item) {
            $("#tooltip").remove();
            var x = item.datapoint[0].toFixed(2);
            var y = item.datapoint[1].toFixed(2);
            var dd = new Date(Math.round(x));
            showTooltip(item.pageX, item.pageY,
                item.series.label + " at " + $.plot.formatDate(dd, "%H:%M:%S") + " was " + y);
        } else {
            $("#tooltip").remove();
        }
    }
 

    var debug = function(str) {
        if ($("#debugEntries li").length > 10) {
            $("#debugEntries li:last").remove();
        }
        $("#debugEntries li:first").before("<li><pre>" + str + "</pre>");
    };

    var client = Stomp.client("ws://is.beeroclock.net:8080");
    client.debug = debug;

    var connectErrorCount = 0;

    // callback for historical data received from the server,
    // needs to populate the current
    var onreceive_historical = function(olddata) {
        var flotd = []
        for (var row in olddata) {
        //    flotd.push({label : data[row]['type'] + "_" + row, data : data[row]['data']})
            //alert("received a row of data for node " + olddata[row]['node'] + "type " + olddata[row]['type'] + "channel " + olddata[row]['channel']);
            stype = olddata[row]['type'];
            channel = olddata[row]['channel'];
            oldkey = make_key(stype, channel);
            oldnode = olddata[row]['node'];
            data[oldnode].push({
                    key : oldkey,
                    label : (config.sensors[stype] + "-chan" + channel),
                    data: olddata[row]['data'],
                    yaxis : config.yaxis[stype]
                    });
        }
    };

    var onreceive =  function(message) {
        var hp = jQuery.parseJSON(message.body);
        connectErrorCount = 0;
        var done = false;
        
        
        if (data[hp.node]) {
            for (var i in hp.sensors) {
                // need to look for the right array element....
                for (var q in data[hp.node]) {
                    var stype = hp.sensors[i].type;
                    var ikey = make_key(stype, i);
                    if (data[hp.node][q].key == ikey) {
                        data[hp.node][q].data.push([Math.round(hp.time_received *
                            1000), hp.sensors[i].value]);
                    }
                }
            }
            done = true;
        }
        
        if (!done) {
            // ok,first time for this node, need to add it outright...
            debug(" didn't find this node, needed to initialise the data structures for this node.");
            data[hp.node] = [];
            // should go and load up the data from alexandria here...
            $.ajax({
                url: "http://karlnet.beeroclock.net/bottle/data/" + hp.node,
                dataType: 'jsonp',
                data: "",
                success: onreceive_historical
            });

            //data[hp.node].node = hp.node;
            // just let the historical loader load it up, and skip this packet...
            // Should really let the new packet still load it in....
//            for (var j in hp.sensors) {
//                stype = hp.sensors[j].type;
//                data[hp.node].push({
//                    key : make_key(stype, j),
//                    label : (config.sensors[stype] + "-chan" + j),
//                    data: [[Math.round(hp.time_received * 1000), hp.sensors[j].value]],
//                    yaxis : config.yaxis[stype]
//                    });
//            }
        }

        // finished with the packet, now update the graphs.
        
        for (var k in data) {
            var newDiv = "<div id='" + k + "' class='node'><h3>" + config.nodes[k] + "</h3><div class='graph'/></div>";
            var initial = $("#placeholder");
            if (initial.length > 0) {
                initial.replaceWith(newDiv);
            } else {
                if ($("#" + k).length === 0) {
                    $("#graphs").append(newDiv);
                }
            }
            // now go and get the graph!
            var graph = $("#" + k + " div");
            graph.bind("plothover", makeTooltip);
            $.plot(graph, data[k], options);
        }
    };

    var onconnect = function(frame) {
        debug("Connected to stomp");
        client.subscribe("/topic/karlnet.>", onreceive);
    };

    var onerror = function(frame) {
        if (frame.message) {
            debug("bang something went wrong: " + frame.message);
        } else {
            // couldn't connect at all, try a few times then give up?
            debug("Failed to connect: " + frame);
            connectErrorCount += 1;
            if (connectErrorCount < 5) {
                client.connect("guest", "password", onconnect, onerror);
            } else {
                debug("Aborting connection after 5 attempts");
            }
        }
    };

    client.connect("guest", "password", onconnect, onerror);

});
