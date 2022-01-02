const path = require('path');

const express = require('express');
const app = express();
app.use(express.static('public'));

const server = require('http').createServer(app);
const io = require('socket.io')(server);
//const p5 = require('p5')(server);
const dgram = require('dgram');

//require { Bundle, Client } from 'node-osc';
Bundle = require('node-osc').Bundle;
Client = require('node-osc').Client;
const client = new Client('192.168.0.128', 8000);

let myIP = "42";

let CCDatas = new Array (3); // For midi CC

let mstr = new Array (79); // mstr[0-3] (channel) // mstr[4-7] (note) // mstr[8-11] (note duration) // mstr[12-13] (bar) // mstr[14] (mute) // mstr[15-79](steps)
for (let i = 0; i < mstr.length; i++) {
    mstr[i] = false; // instantiate everything as false
    // console.log(mtmss[i]);
    }

function fmod(a, b){
    var x = Math.floor(a/b);
    return a - b*x;
    }

io.on('connection', function(client){
    client.on('event', function(data){});
    client.on('disconnect', function(){});
});

const abletonlink = require('abletonlink');
const link = new abletonlink(bpm = 60.0, quantum = 4.0, enable = true);

link.on('numPeers', (numPeers) => console.log("numPeers", numPeers));
link.on('playState', (playState) => console.log("playState", playState));

(() => {
    let lastBeat = 0.0;
    let curr_beat_time = 0.0;
    let prev_beat_time = 0.0;
    let prev_step = 0.0;
    let curr_step = 0.0;
    //let curr_phase;
    link.quantum = 4.0;
    link.isLinkEnable = true;
    link.isPlayStateSync = true;
    link.enablePlayStateSync;
    
    var test = link.isPlayStateSync;
    console.log('link.isPlayStateSync : '+test);
    
     link.startUpdate(4, (beat, phase, bpm, playState) => { // playState // changed update freq from 60 to 8
        //console.log("curr_beat_time : "+beat);
        curr_beat_time = beat;
        //maPhase = phase;
        //console.log("Phase : "+ phase);
        prev_phase = fmod(prev_beat_time,4);
        //console.log("prev_phase : "+ prev_phase);
        prev_step = Math.floor(prev_phase * 4);
        //console.log("prev_step : "+prev_step);
        if(prev_step != curr_step){
            curr_step = prev_step;
            io.emit('step', { curr_step });
            io.send(curr_step);
            
           //console.log("            curr_step : "+curr_step);
        }

        beat = 0 ^ beat;
        //console.log("beat? : "+beat);
        if(0 < beat - lastBeat) {
            io.emit('beat', { beat, phase, bpm, playState }); // playState returns 'undefined' regardless
            console.log("beat? : "+beat);  
            lastBeat = beat;
            }
         //console.log(link.bpm);
         numUsers = link.numPeers;
         io.emit('numUsers',{ numUsers });
         test = link.isPlayStateSync;
         io.emit('test',{test});
         
         prev_beat_time = curr_beat_time;
    });
    
})()

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, "public", "index.html"));
    //res.sendFile(path.join(__dirname, "public", "libraries","p5.min.js"));
});

// app.use("/public", express.static(__dirname + "/public"));


io.on('connection', (socket) => {  // start listening from events from the socket upon connection
    console.log('a user connected');

    socket.on('CCData', (data) => {
      
        // mstr[0-3] (channel) // mstr[4-7] (note) // mstr[8-11] (note duration) // mstr[12-13] (bar) // mstr[14] (mute) // mstr[15-79](steps)
        var s = dgram.createSocket('udp4');
        console.log("data : "+data);
        for (let i = 0; i < data.length; i++) {
            CCDatas[i] = data[i];
        }
        //s.send(Buffer.from(mstr), 3333, '192.168.1.239'); // 
        // a bundle without an explicit time tag
        const bundle = new Bundle(['/CC1', CCDatas[1]], ['/CC2', CCDatas[2]]);
        client.send(bundle);
        s.send(Buffer.from(CCDatas), 3333, '192.168.0.100'); // 

        console.log("CCData envoyé : "+CCDatas);

        });
    
    socket.on('interface', (data) => {
      
        
        // mstr[0-3] (channel) // mstr[4-7] (note) // mstr[8-11] (note duration) // mstr[12-13] (bar) // mstr[14] (mute) // mstr[15-79](steps)
        var s = dgram.createSocket('udp4');
        // console.log("data : "+data);
        for (let i = 0; i < data.length; i++) {
            mstr[i] = data[i];
        }
        s.send(Buffer.from(mstr), 3333, '192.168.0.100'); // 
        //s.send(Buffer.from(mstr), 10000, '192.168.0.101'); // 
        console.log("mstr envoyé : "+mstr);

        });

    socket.on('chBPM', (data) => {
        //var s = dgram.createSocket('udp4');
        //s.send(Buffer.from('bpm 90'), 3333, '192.168.1.239');
        console.log(data.bpm);
        console.log('current bpm : '+link.bpm);
        if (data.bpm > 0) {
            link.bpm++;
            } else {
            link.bpm--;
            }
            
        });

    socket.on('startStop', (data) => {
        console.log('link.playstate : '+link.playstate);
            link.playState = true;
            console.log(data);
        }); 

    
    });

    'use strict';

    ///////// Get the computer's external IP ////////
const os = require('os');

let networkInterfaces = os.networkInterfaces();

let nonLocalInterfaces = {};
for (let inet in networkInterfaces) {
  let addresses = networkInterfaces[inet];
  for (let i=0; i<addresses.length; i++) {
    let address = addresses[i];
    console.log("address : "+address.address);

    if(address.address.includes("192")){
        //console.log("bingo");
        myIP = address.address;
    }

    if (!address.internal) {
      if (!nonLocalInterfaces[inet]) {
        nonLocalInterfaces[inet] = [];
      }
      nonLocalInterfaces[inet].push(address);
    }
  }
}

console.log(nonLocalInterfaces);

server.listen(8082, myIP, () => {
//server.listen(8080, "172.20.10.2", () => {
    console.log("access to "+myIP+":8082 !!");
});