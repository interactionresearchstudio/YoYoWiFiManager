function init() {
    if ("WebSocket" in window) {
        var url = "ws://" + location.host + ":81";
        console.log(url);
        var ws = new WebSocket(url);

        ws.onopen = function() {
            ws.send("Message to send");
            console.log("Message is sent...");
        };

        ws.onmessage = function (evt) { 
            console.log(evt.data);
        };

        ws.onclose = function() { 
            console.log("Connection is closed..."); 
        };
    } 
    else {
        // The browser doesn't support WebSocket
        console.log("WebSocket NOT supported by your Browser!");
    }
}