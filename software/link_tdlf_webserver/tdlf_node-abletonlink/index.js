const path = require('path');

/* node-abletonlink
├── abletonlink@0.1.1
├── dgram@1.0.1
├── express@4.17.1
├── mdns@2.7.2
├── node-osc@6.1.11
├── osc@2.4.4
├── p5@1.3.1
├── socket.io@1.7.4
├── webmidi@2.5.1
└── ws@8.13.0
 */
//var mdns = require('mdns'); // Not implement yet
//var browser = mdns.createBrowser(mdns.tcp('http')); // Not implemented yet

//var osc = require("osc");

/* var udpPort = new osc.UDPPort({
    // This is the port we're listening on.
    localAddress: "127.0.0.1",
    localPort: 57121,

    // This is where sclang is listening for OSC messages.
    remoteAddress: "192.168.50.152",
    remotePort: 3333,
    metadata: true
});

udpPort.on("error", function (err) {
    console.log(err);
});

// Open the socket.
udpPort.open();

////

udpPort.on("ready", function () {
    console.log("on se rend ici");
});

*/


const express = require('express');
const app = express();
app.use(express.static('public'));

const server = require('http').createServer(app);
const io = require('socket.io')(server);
const dgram = require('dgram'); // To connect to the tdlf module

//require { Bundle, Client } from 'node-osc';
Bundle = require('node-osc').Bundle;
Client = require('node-osc').Client;
const client = new Client('192.168.0.128', 8000);

let myIP = "42";
let myUsers = [];
let myUsersLength;
let myUsersAndVotesLength;
const myUsersAndVotes = {}; // socketid:vote // Keep id's and votes together
let proposedBPM;
let countDown;
let counting = 16;

let scaleTracker = Math.floor(Math.random()*8);
//console.log('scaleTracker : ' +scaleTracker);
let scales = [
    ['G','k67','k69','k71','k72','k74','k76','k78','k79'],
    ['Gm','k67','k69','k70','k72','k74','k75','k77','k79'],
    ['G#','k68','k70','k72','k73','k75','k77','k79','k80'],
    ['G#m','k68','k70','k71','k73','k75','k76','k78','k80'],
    ['A', 'k69', 'k71', 'k73', 'k74', 'k76', 'k78', 'k80', 'k81'],
    ['Am', 'k69', 'k71', 'k72', 'k74', 'k76', 'k77', 'k79', 'k81'],
    ['B', 'k71', 'k73', 'k75', 'k76', 'k78', 'k80', 'k82', 'k83'],
    ['Bm', 'k71', 'k73', 'k74', 'k76', 'k78', 'k79', 'k81', 'k83'],
    ['C', 'k72', 'k74', 'k76', 'k77', 'k79', 'k81', 'k83', 'k60'],
    ['Cm', 'k60', 'k62', 'k63', 'k65', 'k67', 'k68', 'k70', 'k72'],
    ['D', 'k62', 'k64', 'k66', 'k67', 'k69', 'k71', 'k73', 'k74'],
    ['Dm', 'k62', 'k64', 'k65', 'k67', 'k69', 'k70', 'k72', 'k74'],
    ['E', 'k64', 'k66', 'k68', 'k69', 'k71', 'k73', 'k75', 'k76'],
    ['F', 'k65', 'k67', 'k69', 'k70', 'k72', 'k74', 'k76', 'k77'],
    ['Fm', 'k65', 'k67', 'k68', 'k70', 'k72', 'k73', 'k75', 'k77']
];

let maGamme = scales[scaleTracker];


//console.log("scales test : "+scales[1][0]);

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
            //console.log("beat? : "+beat);  
            lastBeat = beat;
            if(countDown == true){
                counting = counting-1;
                console.log('counting = '+counting);
                //io.broadcast.emit('counting',{counting});
                io.emit('counting',{counting}); 
    
                if(counting <=0){
                    countDown = false;
                    counting = 16;
                    link.setBpm(parseInt(proposedBPM));
                    console.log('new bpm : '+link.bpm);
                } 
            }
            }
         //console.log(link.bpm);
         //numUsers = link.numPeers;
         //io.emit('numUsers',{ numUsers });
         test = link.isPlayStateSync;
         io.emit('test playState',{test});

         prev_beat_time = curr_beat_time;
    });
    
})()

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, "public", "index.html"));
    //res.sendFile(path.join(__dirname, "public", "libraries","p5.min.js"));
});

// app.use("/public", express.static(__dirname + "/public"));


io.on('connection', (socket) => {  // start listening from events from the socket upon connection
    console.log("New user connected : "+socket.id);
    myUsersAndVotes[socket.id] = 0; // Add the new socket.id as value to a user and initialize vote to '0'
    //cconsole.log("Object.keys : "+Object.keys(myUsersAndVotes));
    for (const key of Object.keys(myUsersAndVotes)) { 
        console.log(key + " : "+myUsersAndVotes[key]);

        };

        myUsersAndVotesLength = Object.keys(myUsersAndVotes).length;
        console.log('myUsersAndVotes length '+myUsersAndVotesLength);
        io.emit('myUsers', { myUsersAndVotesLength });
        console.log("maGamme : "+maGamme);
        socket.broadcast.emit('newScale',maGamme);
        socket.emit('newScale',maGamme); 

    socket.on('disconnect', function() { 
        console.log(socket.id + ' disconnected');
        delete myUsersAndVotes[socket.id];  //remove user from object
        console.log('deleted '+socket.id+'from myUsersAndVotes');
       
        myUsersAndVotesLength = Object.keys(myUsersAndVotes).length;
        console.log('myUsersAndVotes length '+myUsersAndVotesLength);
        io.emit('myUsers', { myUsersAndVotesLength });
        }
    );
    // We need to keep track of how many clients are connected and send them their own id

    socket.on('CCData', (data) => {
      
        // mstr[0-3] (channel) // mstr[4-7] (note) // mstr[8-11] (note duration) // mstr[12-13] (bar) // mstr[14] (mute) // mstr[15-79](steps)
        var s = dgram.createSocket('udp4');
        console.log("data testing : "+data);
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

    socket.on('propose', (data) => {
        console.log("data : "+data);
        proposedBPM = data;
        socket.broadcast.emit('proposedBPM',data);
        socket.emit('proposedBPM',data); 
        });

    socket.on('voting', (data) => {
        console.log("data vote : "+data);
        console.log("socket.id :"+socket.id); 
        myUsersAndVotes[socket.id] = data; // Register vote, add check on id if the user has voted already!
        
        // Add all the votes together and return the means 
        let voteTotal = 0;
        let voteResult;

        for (const key of Object.keys(myUsersAndVotes)) { 
            console.log(key + " : "+myUsersAndVotes[key]);
            voteTotal = voteTotal + myUsersAndVotes[key];
            };

        console.log('voteTotal : '+voteTotal);

        voteResult = voteTotal / myUsersAndVotesLength;

        console.log('voteResult : '+voteResult);

        if(voteResult >= 0.5){
            console.log('we have a successful vote for the suggested BPM of : '+proposedBPM);
            socket.broadcast.emit('changingBPM',proposedBPM); // Send out the new BPM
            socket.emit('changingBPM',proposedBPM); // To the recent client as well...
        }
        countDown = true;  // if yes, change bpm (16 steps countdown)
        });

    socket.on('newScale', (data) => {
            console.log("newScale data : "+data);
            maGamme = data;

            console.log("maVieilleGamme : "+maGamme);

            for(i=0;i<scales.length;i++){
                console.log("i : "+i);
                if (scales[i][0] == "Fm"){
                    console.log("should go back to G!");
                    maGamme = scales[0]; // "back to G"
                    socket.broadcast.emit('newScale',scales[0]);
                    socket.emit('newScale',scales[0]); 
                    break;
                }
                else if (scales[i][0] == maGamme){
                    maGamme = scales[i+1];
                    socket.broadcast.emit('newScale',scales[i+1]);
                    socket.emit('newScale',scales[i+1]); 
                    break;
                }   
            }
            
        });
    
    socket.on('interface', (data) => {
        /* var msg = {
            address: "/oscjs",
            args: [
                {
                    type: "f",
                    value: Math.random()
                },
                {
                    type: "f",
                    value: Math.random()
                }
            ]
        };
     */
       /// console.log("Sending message", msg.address, msg.args, "to", udpPort.options.remoteAddress + ":" + udpPort.options.remotePort);
       /// udpPort.send(msg);
        
        // mstr[0-3] (channel) // mstr[4-7] (note) // mstr[8-11] (note duration) // mstr[12-13] (bar) // mstr[14] (mute) // mstr[15-79](steps)
        var s = dgram.createSocket('udp4');
        console.log("data : "+data);
            for (let i = 0; i < data.length; i++) {
                mstr[i] = data[i];
        }
        s.send(Buffer.from(mstr), 3333, '192.168.50.152');
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
            console.log('link.isplaying : '+link.isPlaying);
            
            if (data == true){
                console.log('startStop is : ' +data);
                link.start; // try to do stg about playing or stopping!
            } else {
                console.log('startStop is : ' +data);
                link.stop;
            }
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
