const path = require('path');

const app = require('express')();
const server = require('http').createServer(app);
const io = require('socket.io')(server);


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
            console.log("curr_step : "+curr_step);

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
         prev_beat_time = curr_beat_time;
    });
    
})()

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, "public", "index.html"));
});


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

server.listen(8080, "192.168.0.128", () => {
    console.log("access to http://192.168.0.128:8080 !!");
});
