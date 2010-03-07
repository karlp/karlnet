// Pretty much from: http://jmesnil.net/stomp-websocket/doc/

var client = Stomp.client("ws://egri:61614");


/*
client.debug = function(str) {
    $("#debug").append(str + "\n");
}
*/



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

client.connect("blah", "blah", onconnect);
