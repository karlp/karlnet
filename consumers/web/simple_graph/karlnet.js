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
    var data = [];
    var dataMap = [];
    var placeholder = $("#placeholder");
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
             { label: 'node1', data: [[14,25],[15,22],[16,19],[17,1]]} ,
             { label: 'node2', data: [[13,16],[13.5,14],[14,12]]},
            ]
    
    
    
    $.plot(placeholder, data, options);


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
        hp = jQuery.parseJSON(message.body);
        connectErrorCount = 0;
        var done = false;
        // look for the right blob of real data to append to...
        for (var current = 0; current < data.length; current++) {
            if (data[current]['label'] == hp.node) {
                rxIndex[hp.node]++;
                data[current]['data'].push([rxIndex[hp.node], hp.sensors[0].value]);
                done = true;
            }
        }
        if (!done) {
            // ok,first time for this node, need to add it outright...
            debug(" didn't find this node, needed to initialise the data structures for this node.");
            data.push({label: hp.node, data: [[0, hp.sensors[0].value]]});
            rxIndex[hp.node] = 1;
        }
        
        $.plot(placeholder, data, options);
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
