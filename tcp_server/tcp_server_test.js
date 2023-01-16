var net = require('net');
const fs = require('fs');
const port = 8000;
const host = '127.0.0.1';
let gpsString = undefined;

const server = net.createServer();
server.listen(port, host, () => {
    console.log('TCP Server is running on port ' + port + '.');
});


let sockets = [];

server.on('connection', function (sock) {
    console.log('CONNECTED: ' + sock.remoteAddress + ':' + sock.remotePort);
    sockets.push(sock);
    sock.on('data', function (data) {
        console.log('DATA ' + sock.remoteAddress + ': ' + data);
        gpsString = data.toString();
        console.log(gpsString);
        fs.writeFile("data.txt", gpsString, (err) => {
            if(err)
                console.log(err);
        });
    });
});
