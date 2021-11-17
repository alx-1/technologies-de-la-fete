var dgram = require('dgram');
var s = dgram.createSocket('udp4');
s.send(Buffer.from('abc'), 3333, 'localhost');
