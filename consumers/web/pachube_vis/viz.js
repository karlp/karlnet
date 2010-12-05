function createViz(divid, feed, datastream, width, height, color, min, max) {
    var maxmin = "";
    if (arguments.length == 8) maxmin = "scaleType:'allmaximized',min:" + min + ",max:" + max + ",";
    if (typeof(google) == 'undefined') {
        document.write("<script type=\"text/javascript\" src=\"http://www.google.com/jsapi\"></script>");
    }
    document.write("<script type=\"text/javascript\">" 
        + " google.load(\'visualization\',\'1\',{packages:[\'annotatedtimeline\']});"
        + " function drawVisualization() {"
          +"  var headID = document.getElementsByTagName('head')[0]; "
          +"  var newScript = document.createElement('script'); "
          +"  newScript.type = 'text/javascript'; "
          +"  newScript.src = 'http://apps.pachube.com/history/archive_json.php?f=", 
                feed, "&d=", datastream, "&callback=process", divid,"'; "
          +"  headID.appendChild(newScript);"
        +" }"
        +" function process", divid, "(archive){ "
        +"    var d; var val; "
        +"    var data = new google.visualization.DataTable(); "
+"            data.addColumn('datetime', 'Date');"
+"            data.addColumn('number', 'Datastream value'); "
+"            for ( var i in archive['time'] ) {"
+"                var timestamp = archive['time'][i];"
+"                if (typeof(timestamp) != \"function\") { "
+"                    var thisrow = data.addRow();"
+"                    var ts_parts = timestamp.split('T');"
+"                    var d_parts = ts_parts[0].split('-');"
+"                    var t_parts = ts_parts[1].split(':'); "
+"                    d = new Date(Date.UTC(parseInt(d_parts[0],10), parseInt(d_parts[1],10)-1 ,parseInt(d_parts[2],10), "
+"parseInt(t_parts[0],10), parseInt(t_parts[1],10), parseInt(t_parts[2],10) ) ); "
+"                    val = parseFloat(archive['value'][i]); "
+"                    data.setValue(thisrow, 0, d); "
+"                    data.setValue(thisrow, 1, val); "
+"                } "
+"            } "
+"            var chart = new google.visualization.AnnotatedTimeLine(document.getElementById(\'", divid, "\'));"
+"            chart.draw(data, { displayAnnotations:true, thickness:1,", maxmin, "displayExactValues:true,fill:20,colors:['#", color, "']}); }"
+"            google.setOnLoadCallback(drawVisualization); "
+"</script>");


}
