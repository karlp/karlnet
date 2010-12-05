// use the library of alexandria to read in some historical data
// 

$(function () {
    var options = {
        lines: { show: true },
        points: { show: true },
        xaxis: { mode : 'time' }
    };
    var data = [];
    var placeholder = $("#placeholder");
    
    var success_cb = function(data) {
        var flotd = []
        flotd.push({label : data[0]['type'], data : data[0]['data']})
        flotd.push({label : data[1]['type'], data : data[1]['data']})
        flotd.push({label : data[2]['type'], data : data[2]['data']})
        $.plot(placeholder, flotd, options);
    };

    $.ajax({
        url: "http://karlnet.beeroclock.net/bottle/data/16899",
        dataType: 'jsonp',
        data: "",
        success: success_cb
    });

});
