var peers = [];

function init() {
    updatePeers();
    setInterval(updatePeers, 15000);
}

function updatePeers() {
    $.getJSON('/yoyo/peers', function (json) {
        if(json.length > 0) {
            var newPeers = json.map(i => { return i.IP;});
            newPeers.forEach(ip => { if(!peers.includes(ip)) addPeer(ip); });
            peers.forEach(ip => { if(!newPeers.includes(ip)) removePeer(ip); });

            peers = newPeers;
        }
    });
}

function addPeer(ip) {
    console.log("addPeer > " + ip);

    let peersListDiv = $('#peers-list');

    let span = $('<span></span>');
    span.attr('id', ip);

    let link = $('<a></a>');
    link.attr('href', "http://" + ip + "/");

    let image = $('<img></img>');
    image.attr('src', "http://" + ip + "/yo.svg");
    image.attr('width', 50);    //TODO: this should be in the CSS and this should be a class

    peersListDiv.append(span);
    span.append(link);
    link.append(image);
}

function removePeer(ip) {
    console.log("removePeer > " + ip);

    $('span[id="' + ip + '"]').remove();
}