// simple graphing stomp received values, one line per node, only sensor 0 is used
// 

$(function () {

    if (!window.WebSocket) {
        $("#placeholder").append("Websockets not supported by this browser");
        return;
    }

    var config = {
            nodes : {
                0x4203 : "teensyhumi",
                0x4201 : "tinytemp",
                0xbabe : "dummy producer"
            },
            sensors : {
                36 : "temp (&deg;C)",
                73 : "internalTemp (raw)",
                102 : "humidity (pF)"
            },
            yaxis : {
                36 : 1,
                73 : 2,
                102 : 2
            }
    };

    var options = {
        lines: { show: true },
        points: { show: true },
        xaxis: { mode : 'time' },
        grid: { hoverable: true}
    };
    var data = {}

// directly from flot examples...
function showTooltip(x, y, contents) {
        $('<div id="tooltip" class="tooltip">' + contents + '</div>').css( {
            top: y + 5,
            left: x + 5,
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
        $("#debugEntries li:last").remove()
    }
    $("#debugEntries li:first").before("<li><pre>" + str + "</pre>");
}

var client = Stomp.client("ws://is.beeroclock.net:8080");
client.debug = debug;

var connectErrorCount = 0;
var onreceive =  function(message) {
        var hp = jQuery.parseJSON(message.body);
        connectErrorCount = 0;
        var done = false;
        
        
        if (data[hp.node]) {
            for (var i in hp.sensors) {
                // need to look for the right array element....
                for (var q in data[hp.node]) {
                    var stype = hp.sensors[i].type;
                    if (data[hp.node][q].label == config.sensors[stype]) {
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
            //data[hp.node].node = hp.node;
            for (var i in hp.sensors) {
                var stype = hp.sensors[i].type;
                data[hp.node].push({label : config.sensors[stype],
                         data: [[Math.round(hp.time_received * 1000), hp.sensors[i].value]],
                         yaxis : config.yaxis[stype]});
            }
        }

        // finished with the packet, now update the graphs.
        
        for (var i in data) {
            var newDiv = "<div id='" + i + "' class='node'><h3>" + config.nodes[i] + "</h3><div class='graph'/></div>";
            var initial = $("#placeholder");
            if (initial.length > 0) {
                initial.replaceWith(newDiv);
            } else {
                if ($("#" + i).length == 0) {
                    $("#graphs").append(newDiv);
                }
            }
            // now go and get the graph!
            var graph = $("#" + i + " div");
            graph.bind("plothover", makeTooltip);
            $.plot(graph, data[i], options);
        }
    }

var onconnect = function(frame) {
    debug("Connected to stomp");
    client.subscribe("/topic/karlnet.>", onreceive);
};

var onerror = function(frame) {
    if (frame.message) {
        debug("bang something went wrong: " + frame.message);
    } else {
        // couldn't connect at all, try a few times then give up?
        debug("Failed to connect: " + frame)
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
