// use the library of alexandria to read in some historical data
// and then use stomp to keep updating the graphs..

$(function () {
    var options = {
        lines: { show: true },
        points: { show: true },
        xaxis: { mode : 'time' }
    };
    var data = [];
    
    var success_cb = function(data) {
        var flotd = []
        for (var row in data) {
            flotd.push({label : data[row]['type'] + "_" + row, data : data[row]['data']})
        }
        var placeholder = $("#" + data[0]['node'] + " div.graph");
        $.plot(placeholder, flotd, options);
    };

    $("#graphs div.node").each(function(index) {
        //alert("got" + index + "nodeid = " + $(this).attr("id"));
        $.ajax({
            url: "http://karlnet.beeroclock.net/bottle/data/" + $(this).attr("id"),
            dataType: 'jsonp',
            data: "",
            success: success_cb
        });
    });
});
