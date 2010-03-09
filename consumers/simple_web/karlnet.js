// Pretty much from: http://jmesnil.net/stomp-websocket/doc/

var client = Stomp.client("ws://is.beeroclock.net:61614");


client.debug = function(str) {
    if ($("#debugEntries li").length > 5) {
        $("#debugEntries li:last").remove()
    }
    $("#debugEntries li:first").before("<li><pre>" + str + "</pre>");
    
}




var onreceive = function(message) {
    hp = jQuery.parseJSON(message.body)
    // bah humbug! what's wrong here?!
    //timestamp = new Date(parseInt(message.timestamp));
    timestamp = new Date();
    row = $("#nodes").find("#" + hp.node)
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

var onerror = function(frame) {
    debug("bang!" + frame.headers.message);
    // try and reconnect?,
    // or just present a button to reconnect?
    client.connect("blah", "blah", onconnect, onerror);
};
    

client.connect("blah", "blah", onconnect, onerror);