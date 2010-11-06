// simple graphing stomp received values, one line per node, only sensor 0 is used
// 

$(function () {

$("#adddiv").click(function() {
    var newid = $("#nodevalue").val();
    alert("adding div id: " + newid);
    var newDiv = "<div id='" + newid + "' class='graph'>this is node: " + newid + " </div>";
    var initial = $("#placeholder");
    if (initial.length > 0) {
        alert ("need to remove the placeholder");
        initial.replaceWith(newDiv);
    } else {
        if ($("#" + newid).length > 0) {
            alert("already have a div for this node....")
        } else {
            $("#graphs").append(newDiv);
        }
    }
    
});




});

