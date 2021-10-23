const path = require('path');

//const app = require('express')();

const express = require('express');
const app = express();
app.use(express.static('public'));

const server = require('http').createServer(app);
const io = require('socket.io')(server);
//const p5 = require('p5')(server);

let yesi = "halo welt";
let stringy = "hey";

// For Websockets
const WebSocket = require('ws');
const socketteserver = new WebSocket.Server({
  port: 8000
});


let sockets = [];
socketteserver.on('connection', function(socket) {
  sockets.push(socket);

  // console.log("une nouvelle sockette");

  // When you receive a message, send that message to every socket.
  socket.on('message', function(msg) {
    //console.log(""+msg);

    // send the whole message as data for now 
    stringy = (""+msg);
    //stringy = ("something else is there");
    let resultat = false;
    let total = 0;

    // if string matches part of the string don't send out 

    const mesStrings = ["something","HIP","BIG","RIGHT_ELBOW","ANKLE","KNEE","TOE","HEEL"]; // RIGHT_HIP/LEFT_HIP/RIGHT_BIG_TOE/RIGHT_ELBOW/LEFT_ELBOW/LEFT_BIG_TOE

    for (let i = 0; i < mesStrings.length; i++){

        resultat = false;   
        resultat = stringy.includes(mesStrings[i]);
        // console.log("argh : "+stringy.includes(mesStrings[i]));
        total = resultat+total;

    }

    if(total > 0 ){
        //console.log("filtre"); 
        sockets.forEach(s => s.send("junk"));
    }

    else { // donc la string n'est pas trouvée
        sockets.forEach(s => s.send(msg));
        console.log("sending " +stringy); 
        yesi = stringy;
    }
   
  });

  // When a socket closes, or disconnects, remove it from the array.
  socket.on('close', function() {
    sockets = sockets.filter(s => s !== socket);
  });
});


function fmod(a, b){
    var x = Math.floor(a/b);
    return a - b*x;
    }

io.on('connection', function(client){
    client.on('event', function(data){});
    client.on('disconnect', function(){});
});

const abletonlink = require('abletonlink');
const link = new abletonlink(bpm = 120.0, quantum = 8.0, enable = true);

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
    console.log('test : '+test);
    
     link.startUpdate(4, (beat, phase, bpm, playState) => { // playState // changed update freq from 60 to 8
        //console.log("curr_beat_time : "+beat);
        curr_beat_time = beat;
        prev_phase = fmod(prev_beat_time,4);
        //console.log("prev_phase : "+prev_phase);
        prev_step = Math.floor(prev_phase * 4);
        //console.log("prev_step : "+prev_step);
        if(prev_step != curr_step){
        
            curr_step = prev_step;
            //io.emit('step', { curr_step });
            io.send(curr_step);
            //console.log("curr_step : "+curr_step);

        }

        beat = 0 ^ beat;
        //console.log("beat? : "+beat);
        if(0 < beat - lastBeat) {
            io.emit('beat', { beat, phase, bpm, playState }); // playState returns 'undefined' regardless
            lastBeat = beat;
            }
         //console.log(link.bpm);
         
         numUsers = link.numPeers;
         io.emit('numUsers',{numUsers});
         test = link.isPlayStateSync;
         io.emit('test',{test});
         io.emit('yesi',{yesi}); // 
         
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
    
    socket.on('chBPM', (data) => {
        console.log(data.bpm);
        console.log('current bpm : '+link.bpm);
        if (data.bpm > 0) {
            link.bpm++;
        } else {
            link.bpm--;
        }
        
        });
    });

server.listen(8080, "192.168.1.47", () => {
//server.listen(8080, "172.20.10.2", () => {
    console.log("access to http://192.168.0.100    :8000 !!");
});