static const char *LINK_TAG = "Link";
static const char *MIDI_TAG = "Midi";

static void periodic_timer_callback(void* arg);
esp_timer_handle_t periodic_timer;

bool startStopCB = false; // l'état du callback 
bool changeLink = false;
bool isPlaying = true;
char sequence[8][10] = {{"sequence"},{"seq1"},{"seq2"},{"seq3"},{"seq4"},{"seq5"},{"seq6"},{"seq7"}};
bool mtmss[1264] = {0}; // 79 x 16 géant et flat (save this for retrieval, add button to select and load them)
bool changeBPM;
double newBPM; // send to setTempo();
int myNoteDuration;
float noteDuration; 
double oldBPM; 

///////////// TAP TEMPO //////////////
/// from : https://github.com/DieterVDW/arduino-midi-clock/blob/master/MIDI-Clock.ino
int tapped = 0;
int tapInterval = 0;
int lastTapTime = 0;
int maximumTapInterval = 1500;
int foisTapped = 0;
int minimumTapInterval = 50;
int tapArray[5] = {0};
int tapTotal = 0;
int tappedBPM = 0;
double curr_beat_time;
double prev_beat_time;
int step = 0 ;
int myBar = 0; 
int dubStep = 0;
int stepsLength[4] = {16,32,48,64}; // varies per note 16-64
int currStep = 0;

// Midi
int channel; // 4 bits midi channel (0-15) -> // drums + más
int sensorValue = -42;
int previousSensorValue = 0;
char zeDrums[] = {0x24,0x26,0x2B,0x32,0x2A,0x2E,0x27,0x4B,0x43,0x31}; // midi drum notes in hexadecimal format
char zeDark[] = {0x3D,0x3F,0x40,0x41,0x42,0x44,0x46,0x47}; // A#(70)(0x46), B(71)(0x47), C#(61)(0x3D), D#(63)(0x3F), E(64)(0x40), F(65)(0x41), F#(66)(0x42), G#(68)(0x44)

// Serial midi
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_TXD  (GPIO_NUM_26) // was TTGO touch  pin 4 (GPIO_NUM_17) // 13 // change for GPIO_NUM_17 // GPIO_NUM_26
#define ECHO_TEST_RXD  (GPIO_NUM_19) 
#define BUF_SIZE (1024)
#define MIDI_TIMING_CLOCK 0xF8
#define MIDI_NOTE_OFF 0x80 // 10000000 // 128
#define MIDI_NOTE_ON 0x90
#define MIDI_START 0xFA // 11111010 // 250
#define MIDI_STOP 0xFC // 11111100 // 252
#define MIDI_NOTE_VEL 0x64 // 1100100 // 100 // note on,  // data
#define MIDI_NOTE_VEL_OFF 0x00 // 0 // note off
#define MIDI_SONG_POSITION_POINTER 0xF2
char MIDI_NOTE_ON_CH[] = {0x90,0x91,0x92,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F}; // note on, channel 10, note on, channel 0 // ajouter d'autres séries

// octave : C |C# D |D# E F |F# G|G# A|A# B 

/*
0x24 // 36 // C2 //  Kick
0x26 // 38 // D2 //  Snare
0x2B // 43 // G2 //  Low tom
0x32 // 50 // D3 //  Hi Tom 
0x2A // 42 // F#2 // Closed Hat 
0x2E // 46 // A#2 // Open Hat
0x27 // 39 // D#2 // Clap 
0x4B // 75 // D#5 // Claves 
0x43 // 67 // G4 //  Agogo 
0x31 // 49 // C#3 // Crash
*/

// penser à un système qui réarrange les notes selon les gammes, on a huit notes...
// est-ce que l'utilisateur peut transposer d'octave...sans doute

void IRAM_ATTR timer_group0_isr(void* userParam) // Link
{
  static BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  TIMERG0.int_clr_timers.t0 = 1;
  TIMERG0.hw_timer[0].config.alarm_en = 1;

  xSemaphoreGiveFromISR(userParam, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }
}
 
void timerGroup0Init(int timerPeriodUS, void* userParam) // Link timer
{
  timer_config_t config = {.alarm_en = TIMER_ALARM_EN,
    .counter_en = TIMER_PAUSE,
    .intr_type = TIMER_INTR_LEVEL,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_EN,
    .divider = 80};

  timer_init(TIMER_GROUP_0, TIMER_0, &config);
  timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timerPeriodUS);
  timer_enable_intr(TIMER_GROUP_0, TIMER_0);
  timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_group0_isr, userParam, 0, nullptr);

  timer_start(TIMER_GROUP_0, TIMER_0);
}

// callbacks
void tempoChanged(double tempo) {
    ESP_LOGI(LINK_TAG, "tempochanged");

    double midiClockMicroSecond = ((60000 / tempo) / 24) * 1000;

    esp_timer_handle_t periodic_timer_handle = (esp_timer_handle_t) periodic_timer;
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer_handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handle, midiClockMicroSecond));
}

void startStopChanged(bool state) {
  // received as soon as sent, we can get the state of 'isPlaying' and use that
  // need to wait for phase to be 0 (and deal with latency)
  startStopCB = state;
  changeLink = true;

  //ESP_LOGI(LINK_TAG, "startstopCB is : %i", state);
  //ESP_LOGI(LINK_TAG, "changeLink is : %i", state);
}

void delayer(int del) {
  delset = esp_timer_get_time()+del;
  ESP_LOGI(TAP_TAG, "delaying until : %i", delset); 
}

static void periodic_timer_callback(void* arg)
{
    char zedata[] = { MIDI_TIMING_CLOCK };
    // ESP_LOGI(LINK_TAG, "MIDI_TIMING_CLOCK");
    uart_write_bytes(UART_NUM_1, zedata, 1);

    // 24 times per second
    // faire modulo 6 pour arriver à 4 ticks par beat
    
    if(CV_TIMING_CLOCK % 6 == 0 ){
      gpio_set_level(CV_5, 1); // CV Clock Out
      // ESP_LOGI(CV_TAG, "CLOCK 1");
      CV_TIMING_CLOCK = 0;
    } else {
      gpio_set_level(CV_5, 0); 
      // ESP_LOGI(CV_TAG, "CLOCK 0");
    }  
    CV_TIMING_CLOCK = CV_TIMING_CLOCK+1;
    // ESP_LOGI(CV_TAG, "CV_TIMING_CLOCK, %i", CV_TIMING_CLOCK);

}

// ableton task
void tickTask(void* userParam)
{
  // connect link
  ableton::Link link(120.0f);
  link.enable(true);
  link.enableStartStopSync(true); // if not no callback for start/stop

  // callbacks
  link.setTempoCallback(tempoChanged);
  link.setStartStopCallback(startStopChanged);

  // phase
  while (true)
  { 
    xSemaphoreTake((QueueHandle_t)userParam, portMAX_DELAY);

    const auto state = link.captureAudioSessionState(); 
    isPlaying = state.isPlaying();
    //  ESP_LOGI(LINK_TAG, "isPlaying : , %i", isPlaying);  
    //const auto phase = state.phaseAtTime(link.clock().micros(), 1); 
    //ESP_LOGI(LINK_TAG, "tempoINC : %i", tempoINC);
    //ESP_LOGI(LINK_TAG, "tempoDEC : %i", tempoDEC);
    //ESP_LOGI(LINK_TAG, "saveSeqConf : %i", saveSeqConf);
    //ESP_LOGI(LINK_TAG, "toTapped : %i", toTapped );

    if ( saveSeqConf == true ) {
      /////// WRITING mtmss[] TO NVS //////
      nvs_handle wificfg_nvs_handler;
      nvs_open("mtmss", NVS_READWRITE, &wificfg_nvs_handler);
      nvs_set_blob(wificfg_nvs_handler,sequence[0],mtmss,1264);
      nvs_commit(wificfg_nvs_handler); 
      nvs_close(wificfg_nvs_handler); 
      ////// END NVS ///// 
      seqSaved = true;
      delset = esp_timer_get_time()+1500000;
      saveSeqConf = false;
      for (int i = 0; i < 180; i++) { // What are we writing?
       ESP_LOGI(SOCKET_TAG, "savedSeq[%i] = %i ", i, mtmss[i]);   
      } 
      ESP_LOGI(NVS_TAG, "Sequence saved"); 
    }

    if ( loadSeqConf == true ) {
      /// NVS READ CREDENTIALS ///
       loadSeqConf = false;
      nvs_handle wificfg_nvs_handler;
      size_t len;
      nvs_open("mtmss", NVS_READWRITE, &wificfg_nvs_handler);
      nvs_get_blob(wificfg_nvs_handler, sequence[selectedSeq], NULL, &len);
      char* mySeq = (char*)malloc(len); 
      nvs_get_blob(wificfg_nvs_handler, sequence[selectedSeq], mySeq, &len);
      nvs_close(wificfg_nvs_handler);
      ///// END NVS /////
      for (int i = 0; i < 242; i++) { // Populate the tmp loadedSequence from nvs
        loadedSeq[i] = mySeq[i];
        ESP_LOGI(SOCKET_TAG, "loadedSeq[%i] = %i ", i, loadedSeq[i]);   
      } 
      seqLoaded = true;
      seqToLoad = true;
      delset = esp_timer_get_time()+2000000;
      ESP_LOGI(NVS_TAG, "Sequence #%i loaded", selectedSeq);
    }

    if ( changeBPM == true ) {
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(newBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      
      link.commitAppSessionState(mySession); 
      const auto tempo = state.tempo(); // quelle est la valeur de tempo?
      myNoteDuration = ((60/(tempo*4))*1000)*noteDuration; // calculate noteDuration as a function of BPM // 60 BPM * 4 steps per beat = 240 steps per minute // 60 seconds / 240 steps = 0,25 secs or 250 milliseconds per step // ((60 seconds / (BPM * 4 steps per beat))*1000 ms)*noteDuration
      ESP_LOGI(LINK_TAG, "BPM changed %i", int(newBPM));
      oldBPM = newBPM; // store this to avoid spurious calls
    }

    if ( tempoINC == true ) {
      const auto tempo = state.tempo(); // quelle est la valeur de tempo?
      newBPM = tempo + 1;
      ESP_LOGI(LINK_TAG, "BPM changed %i", int(newBPM));
      oldBPM = newBPM; // store this to avoid spurious calls
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(newBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      link.commitAppSessionState(mySession); // le problème est que l'instruction de changer le tempo nous revient
      tempoINC = false;
      saveBPM = false; // as changing the BPM here implies not wanting to tap the tempo in
      myNoteDuration = ((60/(tempo*4))*1000)*noteDuration; // calculate noteDuration as a function of BPM // 60 BPM * 4 steps per beat = 240 steps per minute // 60 seconds / 240 steps = 0,25 secs or 250 milliseconds per step // ((60 seconds / (BPM * 4 steps per beat))*1000 ms)*noteDuration
      //ESP_LOGI(LINK_TAG, "tempoINC : %i", tempoINC);
    }

    if ( tempoDEC == true ) {
      const auto tempo = state.tempo(); // quelle est la valeur de tempo?
      newBPM = tempo - 1;
      ESP_LOGI(LINK_TAG, "BPM changed %i", int(newBPM));
      oldBPM = newBPM; // store this to avoid spurious calls
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(newBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      link.commitAppSessionState(mySession);
      tempoDEC = false;
      saveBPM = false;
      myNoteDuration = ((60/(tempo*4))*1000)*noteDuration; // calculate noteDuration as a function of BPM // 60 BPM * 4 steps per beat = 240 steps per minute // 60 seconds / 240 steps = 0,25 secs or 250 milliseconds per step // ((60 seconds / (BPM * 4 steps per beat))*1000 ms)*noteDuration
    }

    ////////// test start stop send to other clients /////////
    //ESP_LOGI(LINK_TAG, "PRE startStopState is :  %i", startStopState);  
    //ESP_LOGI(LINK_TAG, "PRE startStopCB is :  %i", startStopCB); 

    if ( changePiton && startStopState != startStopCB ){ // if local state is different to the SessionState then send to the session state 
        auto mySession = link.captureAppSessionState();
        const auto timez = link.clock().micros();
        mySession.setIsPlaying(startStopState, timez);
        link.commitAppSessionState(mySession);
      }
      
    if ( changeLink && startStopState != startStopCB ){ // if CB state is different to the local startStopState then resync the latter to the former
       startStopState = startStopCB; // resync 
      }

    if( toTapped == true ){

      tapped = esp_timer_get_time()/1000;
      tapInterval = tapped - lastTapTime;

      // ESP_LOGI(TAP_TAG, "Time at tap: %ld ", tapped);
      ESP_LOGI(TAP_TAG, "Time at tap: %i ", tapped);
      ESP_LOGI(TAP_TAG, "Interval between taps n: %i ", tapInterval );

      if(tapInterval > maximumTapInterval){ // reset tapCounter to zero
        foisTapped = 0;
      }

      if(minimumTapInterval < tapInterval && tapInterval < maximumTapInterval){ // only add a tap if we are within the tempo bounds
        
        foisTapped = foisTapped + 1;
   
        ESP_LOGI(TAP_TAG, "foisTapped %i", foisTapped);

        // add a tapInterval to the sliding values array

        tapArray[foisTapped%5] = tapInterval;

      }
      
      if(foisTapped > 5){  // calculate BPM from tapInterval

        for(int i = 0; i<6; i++){
          tapTotal = tapArray[i] + tapTotal;
          ESP_LOGI(TAP_TAG, "tapped interval at %i, %i", i, tapArray[i]);

        }
        tapTotal = tapTotal / 6;

        tappedBPM = 60L * 1000 / tapTotal;

        ESP_LOGI(TAP_TAG, "tapped BPM %i", tappedBPM);

        tapTotal = 0;
        saveBPM = true;
        saveDelay = true; // appeler une fonction pour remettre les variables à false après un moment
        delayer(3000000);
      }

      lastTapTime = tapped;

      toTapped = false;

    }

    if (esp_timer_get_time() > delset ){
     saveBPM = false;

     saveSeq = false; 
     seqSaved = false;
     
     loadSeq = false;
     seqLoaded = false;
   }

    if ( saveBPM == true && tapeArch == true ) {
      
      auto mySession = link.captureAppSessionState();
      const auto timez = link.clock().micros();
      mySession.setTempo(tappedBPM,timez); // setTempo()'s second arg format is : const std::chrono::microseconds atTime
      link.commitAppSessionState(mySession);
      saveBPM = false;
      tapeArch = false;

    }

    //ESP_LOGI(LINK_TAG, "POST startStopState is :  %i", startStopState);  
    //ESP_LOGI(LINK_TAG, "POST startStopCB is :  %i", startStopCB); 
   
    curr_beat_time = state.beatAtTime(link.clock().micros(), 4);
    const double curr_phase = fmod(curr_beat_time, 4);  // const double

    if ( curr_beat_time > prev_beat_time ) {

      // try sending cc messages here 
      if (sensorValue > -42 && configCC == false){

        char zedata1[] = { MIDI_CONTROL_CHANGE_CH[CCChannel-1] }; // send CC message on midi channel 1 '0xB0'
        uart_write_bytes(UART_NUM_1, zedata1, 1); // this function will return after copying all the data to tx ring buffer, UART ISR will then move data from the ring buffer to TX FIFO gradually.
      
        char zedata2[] = { (char)CCmessage};  //  '0x36' (54) trying for Volca Beats Stutter time
        uart_write_bytes(UART_NUM_1, zedata2, 1); 
      
        char zedata3[] = { (char)sensorValue }; // need to convert sensorValue to hexadecimal! 
        uart_write_bytes(UART_NUM_1, zedata3, 1); 

        if (sensorValue != previousSensorValue){
        // ESP_LOGI(CV_TAG, "CV_PITCH, %i", int(sensorValue));
        ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, int(sensorValue)); // CV_PITCH
        ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
        previousSensorValue = sensorValue;
        }
        
      }
      
      const double prev_phase = fmod(prev_beat_time, 4);
      const double prev_step = floor(prev_phase * 4);
      const double curr_step = floor(curr_phase * 4);
      
      //ESP_LOGI(LINK_TAG, "current step : %f", curr_step); 
      //ESP_LOGI(LINK_TAG, "prev step : %f", prev_step); 
     
      if (abs(prev_phase - curr_phase) > 4 / 2 || prev_step != curr_step) { // quantum divisé par 2

        if( ( curr_step == 0 && changePiton ) || ( curr_step == 0 && changeLink ) ) { // start / stop implementation
            
              if(startStopCB) { // saw a simpler way of doing this!
                char zedata[] = { MIDI_START };
                uart_write_bytes(UART_NUM_1, zedata, 1);
                //uart_write_bytes(UART_NUM_1, 0, 1); // ? produces : E (24513) uart: uart_write_bytes(1112): buffer null
                changeLink = false;
                changePiton = false;
                step = 0; // reset step count
              } 
              else { // on jouait et on arrête // changer pout s'arrêter immédiatement après un stop 
                char zedata[] = { MIDI_STOP };
                uart_write_bytes(UART_NUM_1, zedata, 1);
                //uart_write_bytes(UART_NUM_1, 0, 1);
                changeLink = false;
                changePiton = false;
                }
            }
  

        if (step == 64){ // max steps = 64
          step = 0;
          }

        if(startStopCB){
          SSD1306_SetInverted( &I2CDisplay, 1);
          }
          
        if(!startStopCB){
          SSD1306_SetInverted( &I2CDisplay, 0);  
          }

        const int halo_welt = int(curr_phase);

        const int tmpOH = (int) round( state.tempo() );
        // ESP_LOGI(LINK_TAG, "tempo %i", tmpOH); 

        char phases[5] = "TDLF";
        char tmpOHbuf[20];
        char top[20];
        char ipa[16];

        if ( seqSaved == true ) {
          snprintf(tmpOHbuf, 20 , "Sequence"); 
          snprintf(current_phase_step, 20, "Saved   ");
        }

        else if ( saveSeq == true ) {
          snprintf(tmpOHbuf, 20 , "Sequence"); 
          snprintf(current_phase_step, 20, "Save?   ");
        }

        else if ( seqLoaded == true ) {
          snprintf(tmpOHbuf, 20 , "Sequence"); 
          snprintf(current_phase_step, 20, "%i loaded", selectedSeq);
        }

        else if ( loadSeq == true ) {
          snprintf(tmpOHbuf, 20 , "Sequence"); 
          snprintf(current_phase_step, 20, "Load:+%i-?", selectedSeq);
        }
        
        else if (saveBPM == false){
          snprintf(tmpOHbuf, 20 , "%i BPM", tmpOH );     /////// display BPM + Phase + Step /////////

          if (step < 10){
            snprintf(current_phase_step, 20, " 0%i STP", step);
          }
          else {
            snprintf(current_phase_step, 20, " %i STP", step);
          }

        }
        
        else if (saveBPM == true){ // we have a tapped BPM ready to switch?
          snprintf(tmpOHbuf, 20 , "%i BPM?", tappedBPM ); 
          
          if (step < 10){
            snprintf(current_phase_step, 20, " 0%i STP", step);
          }
          else {
            snprintf(current_phase_step, 20, " %i STP", step);
          }

        }

  
      switch (halo_welt)
      {
        case 0:
        strcpy(phases, "X000");
        break;
        case 1:
        strcpy(phases, "XX00");
        break;
        case 2:
        strcpy(phases, "XXX0"); 
        break;
        case 3:
        strcpy(phases, "XXXX");
        break;
        default:
        ESP_LOGI(LINK_TAG, "phases not assigned correctly"); 
      }

      if (rando){
        snprintf(top, 20, "Rclients:%i  %s", nmbrClients, phases);
        }
        else {
          snprintf(top, 20, "clients:%i  %s", nmbrClients, phases);
        }

        SSD1306_Clear( &I2CDisplay, SSD_COLOR_BLACK );

        SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_7x13);
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_North, top, SSD_COLOR_WHITE ); 

        SSD1306_SetFont( &I2CDisplay, &Font_droid_sans_mono_13x24); // &Font_droid_sans_mono_13x24 // &Font_droid_sans_fallback_15x17
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_Center, tmpOHbuf, SSD_COLOR_WHITE );
        SSD1306_FontDrawAnchoredString( &I2CDisplay, TextAnchor_South, current_phase_step, SSD_COLOR_WHITE );
        SSD1306_Update( &I2CDisplay );  
      

      if (startStopCB){ // isPlaying and did we send that note out? 
        // ESP_LOGI(LINK_TAG, "sizeof mtmss : %i", sizeof(mtmss));
        //// shut off the midi notes that need to be ////
        int monTemps = int(esp_timer_get_time()/1000);
        //ESP_LOGI(MIDI_TAG, "Mon Temps, %i", monTemps);
        //for( int i = 0; i < 8; i++ ) {
        //if( MIDI_NOTES_DELAYED_OFF[i] > 0 && MIDI_NOTES_DELAYED_OFF[i] < monTemps ) {

        if( CV_TRIGGER_OFF[0] > 0 && CV_TRIGGER_OFF[0] < monTemps ) {
          gpio_set_level(CV_18, 0); // DAC_D // CV Trigger
          // gpio_set_level(CV_5, 0); // DAC_B // CV Clock Out
          // gpio_set_level(CV_23, 0); // DAC_C // CV Pitch
        }

        if( MIDI_NOTES_DELAYED_OFF[0] > 0 && MIDI_NOTES_DELAYED_OFF[0] < monTemps ) {
          // ESP_LOGI(MIDI_TAG, "Should attempt to turn this off : %i", i);
          // ESP_LOGI(MIDI_TAG, "Should attempt to turn this off ");
          //char zedata0[] = {MIDI_NOTE_OFF};
          //uart_write_bytes(UART_NUM_1,zedata0,1); // note off before anything
          // char zedata1[] = { MIDI_NOTE_ON_CH[1] }; // défini comme midi channel 1 (testing)
          // uart_write_bytes(UART_NUM_1, zedata1, 1); //uart_write_bytes(UART_NUM_1, "0x90", 1); // note on channel 0
          // char zedata2[] = {MIDI_NOTES[0]}; // tableau de valeurs de notes hexadécimales 
          // //char zedata2[] = {MIDI_NOTES[i]}; // tableau de valeurs de notes hexadécimales 
          // uart_write_bytes(UART_NUM_1, zedata2, 1); 
          // char zedata3[] = {MIDI_NOTE_VEL_OFF};
          // uart_write_bytes(UART_NUM_1,zedata3, 1); // velocité à 0
          }
        
        for(int i = 0; i<1;i++){ // 8 x 79 = 632 faut faire ça pour chaque valeur de note
          //for(int i = 0; i<8;i++){ // 8 x 79 = 632 faut faire ça pour chaque valeur de note
          myBar = 0; // We're working on 16 steps so myBar is always '0'
          dubStep = step%stepsLength[myBar]; // modulo 16 // 32 // 48 // 64
          //ESP_LOGI(LINK_TAG, "nouveau 'dub'Step : %i", dubStep);
          currStep = (i*79)+dubStep+15; // 15 is where the step info starts
          // ESP_LOGI(LINK_TAG, "currStep : %i", currStep);
          /* for(int j = 0; j<79;j++){
              ESP_LOGI(LINK_TAG, "mtmss : %i, %i", j, mtmss[j]);
              }  */ 
          // if (mtmss[currStep] == 1){ // send [midi] note out // mute to be implemented // && !muteRecords[i]){ 
          // steppr = seq[0][0]; // seq[chan][note] // unit_16 convert to binary array?
          // ESP_LOGI(SEQ_TAG, "Note on, %i", steppr);
          // *** so just send a bass drum hit to test sync
          // ESP_LOGI(LINK_TAG, "MIDI_NOTE_ON_CH, %i", channel);
          // ESP_LOGI(LINK_TAG, "Note on, %i", i);
          // ESP_LOGI(LINK_TAG, "Step : , %i", step);
          channel = 10;  // Outputting drums for Banshee
          char midi_msg[3];
          for (int client = 0; client < N_CLIENTS; client++) {
            for (int note = 0; note < N_NOTES_PER_CLIENT; note++) {
              if (seq[client][note] & (1 << dubStep)) { // note is on
                // ESP_LOGI(SOCKET_TAG, "client %d note %d is on at step %d", client, note, step);
                // do random
                if ( rando == true ){
                  uint32_t randomizer = esp_random();
                  // ESP_LOGI(MIDI_TAG, "randomizer : %d", randomizer)
                  uint16_t modRandomizer = uint8_t(randomizer / 100000);
                  ESP_LOGI(MIDI_TAG, "modRandomizer : %d", modRandomizer);
                    if ( randomizer > 128 ){
                      char zedata1[] = { MIDI_NOTE_ON_CH[channel] }; 
                      uart_write_bytes(UART_NUM_1, zedata1, 1); 
                      char zedata2[] = {zeDrums[note]};      
                      uart_write_bytes(UART_NUM_1, zedata2, 1); 
                      char zedata3[] = { MIDI_NOTE_VEL };
                      uart_write_bytes(UART_NUM_1, zedata3, 1); // vélocité
                      }
                    }
                    else {
                      char zedata1[] = { MIDI_NOTE_ON_CH[channel] }; 
                      uart_write_bytes(UART_NUM_1, zedata1, 1); 
                      char zedata2[] = {zeDrums[note]};      
                      uart_write_bytes(UART_NUM_1, zedata2, 1); 
                      char zedata3[] = { MIDI_NOTE_VEL };
                      uart_write_bytes(UART_NUM_1, zedata3, 1); // vélocité
                    }
                    
                  /*
                    midi_msg[0] = MIDI_NOTE_ON & channel;
                    midi_msg[1] = zeDrums[note + 1];
                    midi_msg[2] = MIDI_NOTE_VEL;
                    // uart_write_bytes(UART_NUM_1, midi_msg, sizeof(midi_msg));
                    uart_write_bytes_with_break(UART_NUM_1, midi_msg, sizeof(midi_msg), 128);
                  */
                    //ESP_LOGI(MIDI_TAG, "Sending midi: %d %d %d", zedata1[], zedata2[], zedata3[]);
                    //ESP_LOGI(MIDI_TAG, "Sending midi: %d %d %d", midi_msg[0], midi_msg[1], midi_msg[2]);
                  }
                }
              }
              /*  if (channel == 10){ // are we playing drumz // solenoids ?
                  char zedata1[] = {MIDI_NOTE_ON_CH[channel], zeDrums[0], MIDI_NOTE_VEL}; // Send a test bass drum
                  // char zedata1[] = { MIDI_NOTE_ON_CH[channel] }; // défini comme channel 10(drums), ou channel 1(synth base) pour l'instant mais dois pouvoir changer
                  // uart_write_bytes(UART_NUM_1, zedata1, 1); // this function will return after copying all the data to tx ring buffer, UART ISR will then move data from the ring buffer to TX FIFO gradually.
                  uart_write_bytes_with_break(UART_NUM_1, zedata1, sizeof(zedata1), 128); // Testing bass drum
                  ESP_LOGI(MIDI_TAG, "Sending post midi"); // Testing
                  //char zedata2[] = {zeDrums[i]};      // arriver de 0-8
                  // char zedata2[] = {zeDrums[0]};      // Play a clap on every beat
                  // uart_write_bytes(UART_NUM_1, zedata2, 1); // tableau de valeurs de notes hexadécimales 
                  // char zedata3[] = { MIDI_NOTE_VEL };
                  // uart_write_bytes(UART_NUM_1, zedata3, 1); // vélocité
                  
                  gpio_set_level(CV_18,1); // DAC_D (CV_18) get that note out !    
                  CV_TRIGGER_OFF[0] = int((esp_timer_get_time()/1000)+(myNoteDuration/64)); // set trigger duration
                }

                else if (channel == 5){ // synth, {0x99,0x90};

                  char zedata1[] = { MIDI_NOTE_ON_CH[channel] }; // défini comme midi channel channel 0 
                  // ESP_LOGI(MIDI_TAG, "MIDI_NOTE_ON_CH, %i", MIDI_NOTE_ON_CH[channel]);
                  uart_write_bytes(UART_NUM_1, zedata1, 1); 

                  // char zedata2[] = {zeDark[i]}; // tableau de valeurs de notes hexadécimales 
                  char zedata2[] = {zeDark[0]}; // tableau de valeurs de notes hexadécimales 
                  uart_write_bytes(UART_NUM_1, zedata2, 1); 

                  char zedata3[] = { MIDI_NOTE_VEL };
                  uart_write_bytes(UART_NUM_1, zedata3, 1); // vélocité
                  
                  //const auto tempo = state.tempo(); // quelle est la valeur de tempo?
                  //myNoteDuration = ((60/(tempo*4))*1000)*noteDuration; // calculate noteDuration as a function of BPM // 60 BPM * 4 steps per beat = 240 steps per minute // 60 seconds / 240 steps = 0,25 secs or 250 milliseconds per step // ((60 seconds / (BPM * 4 steps per beat))*1000 ms)*noteDuration

                  // check if there is a value in the array and populate it or just add the note
                  //int posArray = step%8;
                  //ESP_LOGI(MIDI_TAG, "posArray, %i", posArray);

                  //MIDI_NOTES[posArray] = zeDark[i];
                  //MIDI_NOTES_DELAYED_OFF[posArray] = int((esp_timer_get_time()/1000)+myNoteDuration); // duration

                  //MIDI_NOTES[0] = zeDark[i];
                  MIDI_NOTES[0] = zeDark[0];
                  MIDI_NOTES_DELAYED_OFF[0] = int((esp_timer_get_time()/1000)+myNoteDuration); // duration

                  ESP_LOGI(MIDI_TAG, "MIDI NOTE, %i", MIDI_NOTES[0]);
                  ESP_LOGI(MIDI_TAG, " "); // new line
                  // ESP_LOGI(MIDI_TAG, "MIDI TIME, %i", MIDI_NOTES_DELAYED_OFF[0]);

                  }

                  else{
                    ESP_LOGI(MIDI_TAG, "Midi channel other than 0 or 1"); // new line
                    // ajouter d'autres gammes (scales)
                    } 
        */        
                //char zedata3[] = { MIDI_NOTE_VEL };
                //uart_write_bytes(UART_NUM_1, zedata3, 1); // vélocité
                // uart_write_bytes(UART_NUM_1, "0", 1); // ??

              // } // This is the end of the original loop going through the 8 notes
                  
          }
           // ESP_LOGI(LINK_TAG, "");
        }

        if(isPlaying){
          // ESP_LOGI(LINK_TAG, "step : %d", step); 
          step++; // might be off to add '1' right away
          }

      }

    } // fin de if curr_beat_time is > prev_beat_time

    prev_beat_time = curr_beat_time;

    portYIELD();
  } 

} // fin de tick task
