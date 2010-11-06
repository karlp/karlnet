// simple graphing stomp received values, one line per node, only sensor 0 is used
// 

$(function () {

    if (!window.WebSocket) {
        $("#placeholder").append("Websockets not supported by this browser");
        return;
    }

    var options = {
        lines: { show: true },
        points: { show: true },
        xaxis: { tickDecimals: 0, tickSize: 1 }
    };
    var data = {}
    var dataMap = {};
    var rxIndex = {};

    var node1 = { label: 'node1', data: [[11, 25],[12,26],[13,26],[14,25],[15,22],[16,19],[17,1]] } ; 
    var node2 = { label: 'node2', data: [[12, 9],[12.5,10],[13,16],[13.5,14],[14,12]] };
    dataMap['node1'] = node1;
    dataMap['node2'] = node2;

    // dummy...
    dataGood = [ { label: 'node1', data: [[11, 25],[12,26],[13,26],[14,25],[15,22],[16,19],[17,1]]} ,
            { label: 'node2', data: [[12, 9],[12.5,10],[13,16],[13.5,14],[14,12]]}
            ]
    dataBad = [ { label: 'node1', data: [[11, 25],[12,26],[13,26]]},
             { label: 'node2', data: [[12, 9],[12.5,10]]},
             { label: 'node4', data: [[14,25],[15,22],[16,19],[17,1]]} ,
             { label: 'node5', data: [[13,16],[13.5,14],[14,12]]},
            ]

    nodes = ["node1","node2"];
    for (var node in nodes) {
        //var nid = "#" + nodes[node];
        //alert("blah blah blah" + nid);
        //var graph = $(nid);
        //var graph = $("#node1");
        //$.plot(graph, [dataMap[nodes[node]]], options);
    }


    // we need data { [ { label=node1, data=blah} , { label=node2, dataj


 

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
                    if (data[hp.node][q].label == hp.sensors[i].type) {
                        data[hp.node][q].data.push([rxIndex[hp.node], hp.sensors[i].value]);
                    }
                }
            }
            rxIndex[hp.node]++;
            done = true;
        }
        
        if (!done) {
            // ok,first time for this node, need to add it outright...
            debug(" didn't find this node, needed to initialise the data structures for this node.");
            data[hp.node] = [];
            //data[hp.node].node = hp.node;
            for (var i in hp.sensors) {
                data[hp.node].push({label : hp.sensors[i].type, data: [[0, hp.sensors[i].value]]});
            }
            rxIndex[hp.node] = 1;
        }

        // finished with the packet, now update the graphs.
        
        for (var i in data) {
            var newDiv = "<div id='" + i + "' class='graph'></div>";
            var initial = $("#placeholder");
            if (initial.length > 0) {
                initial.replaceWith(newDiv);
            } else {
                if ($("#" + i).length == 0) {
                    $("#graphs").append(newDiv);
                }
            }
            // now go and get the graph!
            var graph = $("#" + i);
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
