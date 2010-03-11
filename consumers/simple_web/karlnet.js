// Pretty much from: http://jmesnil.net/stomp-websocket/doc/


(function(window) {


var debug = function(str) {
    if ($("#debugEntries li").length > 10) {
        $("#debugEntries li:last").remove()
    }
    $("#debugEntries li:first").before("<li><pre>" + str + "</pre>");
}


var client = Stomp.client("ws://is.beeroclock.net:61619");
client.debug = debug;

var onreceive = function(message) {
    hp = jQuery.parseJSON(message.body)
    // bah humbug! what's wrong here?!
    //timestamp = new Date(parseInt(message.timestamp));
    timestamp = new Date();
    row = $("#nodes").find("#node_" + hp.node)
    if (row.length == 0) {
        row = $("#nodes tr:last").clone();
        row.attr("id", hp.node);
        $("#nodename", row).html(hp.node);
        $("#nodes tr:last").after(row);
    }
    row.find('#sensor1-type').html(hp.sensor1.type);
    row.find('#sensor1-value').html(hp.sensor1.value);
    row.find('#sensor2-type').html(hp.sensor2.type);
    row.find('#sensor2-value').html(hp.sensor2.value);
    row.find('#last-seen').html(timestamp.toString());
}

var onconnect = function(frame) {
    debug("Connected to stomp");
    client.subscribe("/topic/karlnet", onreceive);
};

var connectErrorCount = 0;
var onerror = function(frame) {
    if (frame.message) {
        debug("bang something went wrong: " + frame.message);
    } else {
        // couldn't connect at all, try a few times then give up?
        debug("Failed to connect: " + frame)
        connectErrorCount += 1;
        if (connectErrorCount < 5) {
            client.connect("blah", "blah", onconnect, onerror);
        } else {
            debug("Aborting connection after 5 attempts");
        }
    }
};
    

client.connect("blah", "blah", onconnect, onerror);

})(window);
