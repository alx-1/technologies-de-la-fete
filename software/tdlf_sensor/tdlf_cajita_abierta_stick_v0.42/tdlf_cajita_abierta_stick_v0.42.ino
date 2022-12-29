// HARDWARE REQUIREMENTS
// ==================
// Example for ESP32 sending OSC midi to TouchOSC -> Virtual Midi -> DAW
// TODO better calibration, adding a way to fix a CC value, apply filtering
//
// LIBRAIRIES USED : 
// Polyfill analogWrite
// Seed Studio's ADXL345 
// Tof's MicroOSCUDP
// microsmooth
// Adafruit's GFX + SSD1306
// LITTLEFS
//
// Config //
#define Accelerometer true
#define PressureSensor true

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <MicroOscUdp.h>
#include "microsmooth.h"

#include "FS.h"
#include "LITTLEFS.h" 

/* You only need to format LITTLEFS the first time you run a
   test or else use the LITTLEFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
   
#define FORMAT_LITTLEFS_IF_FAILED true

bool configure = false; // A button press will start it
bool recording = false; // Save data while sending to the tdlf server
bool recordingStandAlone = false; // Save data on it's own (message in a bottle)
bool playback = false;
const int arrayLength = 65535; // 65535
byte sensorData[arrayLength] = {0}; // Array to store the values, intialize with 0s
long sensorIndex = 0; // Keep track of the values in the array

File fileToAppend;

//// For a PWM to the LEDPIN on the nodeMCU ESP32
#include <analogWrite.h> // From the polyfill analogWrite library for ESP32 //

//#ifndef LED_BUILTIN
//#define LED_BUILTIN 2 // For the boards that do not have a LED_BUILTIN
//#endif

#if defined Accelerometer
#include <ADXL345.h>
ADXL345 adxl; //variable adxl is an instance of the ADXL345 library
double ax,ay,az; // actual acceleration on all axis
int aIntx, aInty, aIntz; // Lose precision but hey it's MIDI!
int x,y,z; // pan tilt roll
#endif

#if defined PressureSensor
int blowValue = 0;
int suckValue = 0;
uint8_t ccBlow = 0;
uint8_t ccSuck = 0;
uint8_t ccmap = 0;
uint8_t cclast = 0;
uint8_t ccmapunfilter = 0;
float press = 0.00;
float pressmap = 0.00;
long startMillis = 0;
uint16_t *ptr;
#endif

int sensor1Value = 0; 
int sensor2Value = 0; 
int sensor3Value = 0; 
int sensor4Value = 0; 


const char* monAP = "CajitaAbierta";

bool debugSerial = true;

const long period = 5; //time between samples in milliseconds // change from '10' to '5' when not printing to serial
IPAddress sendIp(192, 168, 0, 255); // <- default not really used, we are using Bonjour (mDNS) to find IP and PORT of touchoscbridge
unsigned int sendPort = 12101; // <- touchosc port
bool oscServerFound = false;
bool error = 0;
int statusCode;
int i = 0;
String st;
String content;
String esid;
String epass = "";


////////////// DISPLAY //////////
#include <SPI.h>
//#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
/////////////////////////////

/*
 * This ESP32 button code is created by esp32io.com https://esp32io.com/tutorials/esp32-button-long-press-short-press
 */

#include <ezButton.h>

#define SHORT_PRESS_TIME 1000 // 1000 milliseconds
#define LONG_PRESS_TIME  1000 // 1000 milliseconds

ezButton button(0); // create ezButton object that attach to pin GPIO0 (default button on ESP32s, Node)
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
bool isPressing = false;
bool isLongDetected = false;

/// Accelerometer ///
# if defined Accelerometer
String storedAccelxChan;
String storedAccelxCC;
int storedAccelxChanInt;
int storedAccelxCCInt;

String storedAccelyChan;
String storedAccelyCC;
int storedAccelyChanInt;
int storedAccelyCCInt;

String storedAccelzChan;
String storedAccelzCC;
int storedAccelzChanInt;
int storedAccelzCCInt;

String storedPanChan;
String storedPanCC;
int storedPanChanInt;
int storedPanCCInt;

String storedTiltChan;
String storedTiltCC;
int storedTiltChanInt;
int storedTiltCCInt;

String storedRollChan;
String storedRollCC;
int storedRollChanInt;
int storedRollCCInt;
#endif

/// BlowSuck ///
String storedBlowChan;
String storedBlowCC;
int storedBlowChanInt;
int storedBlowCCInt;

String storedSuckChan;
String storedSuckCC;
int storedSuckChanInt;
int storedSuckCCInt;

/// Sensors
String storedSensor1Chan;
String storedSensor1CC;
int storedSensor1ChanInt;
int storedSensor1CCInt;

String storedSensor2Chan;
String storedSensor2CC;
int storedSensor2ChanInt;
int storedSensor2CCInt;

String storedSensor3Chan;
String storedSensor3CC;
int storedSensor3ChanInt;
int storedSensor3CCInt;

String storedSensor4Chan;
String storedSensor4CC;
int storedSensor4ChanInt;
int storedSensor4CCInt;

bool checkAccel = false;
bool checkPanTiltRoll = false;
bool checkBlow = false;
bool checkSuck = false;
bool checkSensor1 = false;
bool checkSensor2 = false;
bool checkSensor3 = false;
bool checkSensor4 = false;

//Function Declaration
WiFiUDP udp;
WebServer server(80);

bool readCFSensor(byte sensorAddress);
bool readAccelerometer();

bool testWifi(void);
void launchWeb(void);
void setupAP(void);

MicroOscUdp<1024> oscUdp(&udp, sendIp, sendPort);
uint8_t midi[4]; 

//SETUP
void setup() {

  btStop();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  ///////////// DISPLAY /////////////
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.setRotation(2);
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(25, 0);
  display.print(F("Technologies"));// Start at top-left corner
  display.setCursor(30, 16);
  display.println(F("de la fete "));
  
  #if defined Accelerometer
  display.setCursor(0, 35);
  display.println(F("Accel/Press/Sensors"));
  #endif
  
  display.display();
  delay(2000); // Pause for 1 second
  ////////////////////////////////

  
  // INITIATE SERIAL COMMUNICATION
  Serial.begin(115200);
  while (!Serial)
  {
  }
  Serial.println("DEBUG Starting WiFi");

  Wire.begin();

  // Set resolution for a specific pin
  // analogWriteResolution(LED_BUILTIN, 12);

  // filter
  ptr = ms_init(EMA);

  // wifi
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM 

  /*
    // CLEAR EEPROM to reset wifi credentials
    // AP will start (blowsuckAP), visit: http://192.168.4.1
    for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
    }
    */
  

  delay(10);
  
  button.setDebounceTime(50); // set debounce time to 50 milliseconds
 

  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  for (int i = 32; i < 64; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  for (int i = 64; i < 66; ++i)
  {
    storedAccelxChan += char(EEPROM.read(i));
  }
  for (int i = 66; i < 69; ++i)
  {
    storedAccelxCC += char(EEPROM.read(i));
  }

 // y
   for (int i = 69; i < 71; ++i)
  {
    storedAccelyChan += char(EEPROM.read(i));
  }
  for (int i = 71; i < 74; ++i)
  {
    storedAccelyCC += char(EEPROM.read(i));
  }
  
  // z
 for (int i = 74; i < 76; ++i)
  {
    storedAccelzChan += char(EEPROM.read(i));
  }
  for (int i = 76; i < 79; ++i)
  {
    storedAccelzCC += char(EEPROM.read(i));
  }

  // Pan
 for (int i = 79; i < 81; ++i)
  {
    storedPanChan += char(EEPROM.read(i));
  }
  for (int i = 81; i < 84; ++i)
  {
    storedPanCC += char(EEPROM.read(i));
  }

  // Tilt
 for (int i = 84; i < 86; ++i)
  {
    storedTiltChan += char(EEPROM.read(i));
  }
  for (int i = 86; i < 89; ++i)
  {
    storedTiltCC += char(EEPROM.read(i));
  }

  // Roll
 for (int i = 89; i < 91; ++i)
  {
    storedRollChan += char(EEPROM.read(i));
  }
  for (int i = 91; i < 94; ++i)
  {
    storedRollCC += char(EEPROM.read(i));
  }

  // Blow
 for (int i = 256; i < 258; ++i)
  {
    storedBlowChan += char(EEPROM.read(i));
  }
  for (int i = 258; i < 261; ++i)
  {
    storedBlowCC += char(EEPROM.read(i));
  }

  // Suck
 for (int i = 261; i < 263; ++i)
  {
    storedSuckChan += char(EEPROM.read(i));
  }
  for (int i = 263; i < 266; ++i)
  {
    storedSuckCC += char(EEPROM.read(i));
  }

  // Sensors 
 for (int i = 266; i < 268; ++i)
  {
    storedSensor1Chan += char(EEPROM.read(i));
  }
  for (int i = 268; i < 271; ++i)
  {
    storedSensor1CC += char(EEPROM.read(i));
  }

   for (int i = 271; i < 273; ++i)
  {
    storedSensor2Chan += char(EEPROM.read(i));
  }
  for (int i = 273; i < 276; ++i)
  {
    storedSensor2CC += char(EEPROM.read(i));
  }

     for (int i = 276; i < 278; ++i)
  {
    storedSensor3Chan += char(EEPROM.read(i));
  }
  for (int i = 278; i < 281; ++i)
  {
    storedSensor3CC += char(EEPROM.read(i));
  }

     for (int i = 281; i < 283; ++i)
  {
    storedSensor4Chan += char(EEPROM.read(i));
  }
  for (int i = 283; i < 286; ++i)
  {
    storedSensor4CC += char(EEPROM.read(i));
  }
  

  if (debugSerial) {
    Serial.println();
    Serial.println("Reading EEPROM");
    Serial.print("SSID: ");
    Serial.println(esid);
    Serial.print("PASS: ");
    Serial.println(epass);

    storedAccelxChanInt = storedAccelxChan.toInt();
    Serial.print("storedAccelxChanInt : ");
    Serial.println(storedAccelxChanInt);
    storedAccelxCCInt = storedAccelxCC.toInt();
    Serial.print("storedAccelxCCInt : ");
    Serial.println(storedAccelxCCInt);

    storedAccelyChanInt = storedAccelyChan.toInt();
    Serial.print("storedAccelyChanInt : ");
    Serial.println(storedAccelyChanInt);
    storedAccelyCCInt = storedAccelyCC.toInt();
    Serial.print("storedAccelyCCInt : ");
    Serial.println(storedAccelyCCInt);

    storedAccelzChanInt = storedAccelzChan.toInt();
    Serial.print("storedAccelzChanInt : ");
    Serial.println(storedAccelzChanInt);
    storedAccelzCCInt = storedAccelzCC.toInt();
    Serial.print("storedAccelzCCInt : ");
    Serial.println(storedAccelzCCInt);

    storedPanChanInt = storedPanChan.toInt();
    Serial.print("storedPanChanInt : ");
    Serial.println(storedPanChanInt);
    storedPanCCInt = storedPanCC.toInt();
    Serial.print("storedPanCCInt : ");
    Serial.println(storedPanCCInt);

    storedTiltChanInt = storedTiltChan.toInt();
    Serial.print("storedTiltChanInt : ");
    Serial.println(storedTiltChanInt);
    storedTiltCCInt = storedTiltCC.toInt();
    Serial.print("storedTiltCCInt : ");
    Serial.println(storedTiltCCInt);

    storedRollChanInt = storedRollChan.toInt();
    Serial.print("storedRollChanInt : ");
    Serial.println(storedRollChanInt);
    storedRollCCInt = storedRollCC.toInt();
    Serial.print("storedRollCCInt : ");
    Serial.println(storedRollCCInt);

    storedBlowChanInt = storedBlowChan.toInt();
    Serial.print("storedBlowChanInt : ");
    Serial.println(storedBlowChanInt);
    storedBlowCCInt = storedBlowCC.toInt();
    Serial.print("storedBlowCCInt : ");
    Serial.println(storedBlowCCInt);

    storedSuckChanInt = storedSuckChan.toInt();
    Serial.print("storedSuckChanInt : ");
    Serial.println(storedSuckChanInt);
    storedSuckCCInt = storedSuckCC.toInt();
    Serial.print("storedSuckCCInt : ");
    Serial.println(storedSuckCCInt);

    storedSensor1ChanInt = storedSensor1Chan.toInt();
    Serial.print("storedSensor1ChanInt : ");
    Serial.println(storedSensor1ChanInt);
    storedSensor1CCInt = storedSensor1CC.toInt();
    Serial.print("storedSensor1CCInt : ");
    Serial.println(storedSensor1CCInt);

    storedSensor2ChanInt = storedSensor2Chan.toInt();
    Serial.print("storedSensor2ChanInt : ");
    Serial.println(storedSensor2ChanInt);
    storedSensor2CCInt = storedSensor2CC.toInt();
    Serial.print("storedSensor2CCInt : ");
    Serial.println(storedSensor2CCInt);

    storedSensor3ChanInt = storedSensor3Chan.toInt();
    Serial.print("storedSensor3ChanInt : ");
    Serial.println(storedSensor3ChanInt);
    storedSensor3CCInt = storedSensor3CC.toInt();
    Serial.print("storedSensor3CCInt : ");
    Serial.println(storedSensor3CCInt);

    storedSensor4ChanInt = storedSensor4Chan.toInt();
    Serial.print("storedSensor4ChanInt : ");
    Serial.println(storedSensor4ChanInt);
    storedSensor4CCInt = storedSensor4CC.toInt();
    Serial.print("storedSensor4CCInt : ");
    Serial.println(storedSensor4CCInt);

    if(storedAccelxCCInt+storedAccelyCCInt+storedAccelzCCInt > 0){
      checkAccel = true ;
    }
    Serial.print("Checking acceleration? : ");
    Serial.println(checkAccel);
    
    if(storedPanCCInt+storedTiltCCInt+storedRollCCInt > 0){
      checkPanTiltRoll = true;
    }
    Serial.print("Checking orientation? : ");
    Serial.println(checkPanTiltRoll);
   
    if(storedBlowCCInt > 0){
      checkBlow = true;
    }
    Serial.print("Checking blow? : ");
    Serial.println(checkBlow);

   if(storedSuckCCInt > 0){
      checkSuck = true;
    }
    Serial.print("Checking suck? : ");
    Serial.println(checkSuck);

   if(storedSensor1CCInt > 0){
      checkSensor1 = true;
    }
    Serial.print("Checking sensor1? : ");
    Serial.println(checkSensor1);

   if(storedSensor2CCInt > 0){
      checkSensor2 = true;
    }
    Serial.print("Checking sensor2? : ");
    Serial.println(checkSensor2);

   if(storedSensor3CCInt > 0){
      checkSensor3 = true;
    }
    Serial.print("Checking sensor3? : ");
    Serial.println(checkSensor3);

   if(storedSensor4CCInt > 0){
      checkSensor4 = true;
    }
    Serial.print("Checking sensor4? : ");
    Serial.println(checkSensor4);
  }

  WiFi.begin(esid.c_str(), epass.c_str());

  if (!MDNS.begin("blowsuck")) {
      Serial.println("Error setting up MDNS responder!");
      while(1){
          delay(1000);
      }
     
  }

  #if defined Accelerometer
  adxl.powerOn();

  //set activity/ inactivity thresholds (0-255)
  adxl.setActivityThreshold(75); //62.5mg per increment
  adxl.setInactivityThreshold(75); //62.5mg per increment
  adxl.setTimeInactivity(10); // how many seconds of no activity is inactive?
 
  //look of activity movement on this axes - 1 == on; 0 == off 
  adxl.setActivityX(1);
  adxl.setActivityY(1);
  adxl.setActivityZ(1);
 
  //look of inactivity movement on this axes - 1 == on; 0 == off
  adxl.setInactivityX(1);
  adxl.setInactivityY(1);
  adxl.setInactivityZ(1);
 
  //look of tap movement on this axes - 1 == on; 0 == off
  adxl.setTapDetectionOnX(0);
  adxl.setTapDetectionOnY(0);
  adxl.setTapDetectionOnZ(0); // 1 
 
  //set values for what is a tap, and what is a double tap (0-255)
  adxl.setTapThreshold(50); //62.5mg per increment
  adxl.setTapDuration(15); //625us per increment
  adxl.setDoubleTapLatency(80); //1.25ms per increment
  adxl.setDoubleTapWindow(200); //1.25ms per increment
 
  //set values for what is considered freefall (0-255)
  adxl.setFreeFallThreshold(7); //(5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(45); //(20 - 70) recommended - 5ms per increment
 
  //setting all interrupts to take place on int pin 1
  //I had issues with int pin 2, was unable to reset it
  adxl.setInterruptMapping( ADXL345_INT_SINGLE_TAP_BIT,   ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_DOUBLE_TAP_BIT,   ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_FREE_FALL_BIT,    ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_ACTIVITY_BIT,     ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_INACTIVITY_BIT,   ADXL345_INT1_PIN );
 
  //register interrupt actions - 1 == on; 0 == off  
  adxl.setInterrupt( ADXL345_INT_SINGLE_TAP_BIT, 0);
  adxl.setInterrupt( ADXL345_INT_DOUBLE_TAP_BIT, 0);
  adxl.setInterrupt( ADXL345_INT_FREE_FALL_BIT,  0);
  adxl.setInterrupt( ADXL345_INT_ACTIVITY_BIT,   0);
  adxl.setInterrupt( ADXL345_INT_INACTIVITY_BIT, 0);
  #endif

  /// LITTLEFS ///
  if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LITTLEFS Mount Failed");
        return;
    }
  //  LITTLEFS.format(); // Only do this once
  /// End LITTLEFS ///

} // End of Setup

void loop() {

  if ( sensorIndex >= arrayLength-12 ) {  // Because there are 12 data points (could go over the array's memory)
        if (recordingStandAlone){
          recordingStandAlone = 0;
          
          // Start saving the contents of the array to FS
          
      // use the array and save it, line by line with commas between the data points
      Serial.println("Will save to file"); // save to file... appending I guess...

      display.clearDisplay();
      display.setTextSize(2);  // Normal 1:1 pixel scale
      display.setCursor(0, 5);
      display.println(F("Saving to"));// Start at top-left corner
      display.setCursor(0, 25); 
      display.println(F("File"));
      display.setCursor(0,48);
      display.setTextSize(1);  
      display.print(F("Might take a few mins"));
      display.display();
     
      LITTLEFS.remove("/sensorData.txt"); // Currently only that one file...
      
      File fileToAppend = LITTLEFS.open("/sensorData.txt", "a");       // open file
      
      for ( int i = 0 ; i < arrayLength; i++){ // Change this back to 65535
        
          fileToAppend.println(sensorData[i]);
          delay(2);
          //Serial.print("sensorData[i] : "); 
          //Serial.println(sensorData[i]);
          //fileToAppend.println(i);
          //fileToAppend.print(',');
          //Serial.println("doing it");     
      }
      
      fileToAppend.close();

      display.clearDisplay();
      display.setTextSize(2);  // Normal 1:1 pixel scale
      display.setCursor(0, 5);
      display.println(F("File saved"));// Start at top-left corner
      display.setCursor(0,48);
      display.setTextSize(1);  
      display.print(F("After party reboot!"));
      display.display();
      delay(3000);
          // End saving routine
          
          // Start configure
          configure = 0;
        }
      
        sensorIndex = 0; // Loop back throught the array
        
      } 

      Serial.print("sensorIndex : ");
      Serial.println(sensorIndex);
   
  if ( playback ) {
    delay(40); // This is where the BPM argument eventually goes in
   
  }

  if (debugSerial ) {
    Serial.print("configure : ");
    Serial.println(configure);
    Serial.print("playback : ");
    Serial.println(playback);
    Serial.print("recording : ");
    Serial.println(recording);
    Serial.print("recordingStandAlone : ");
    Serial.println(recordingStandAlone);
    Serial.println(" ");
    }
    
///// Button detect short or long presses /////  
  button.loop(); // MUST call the loop() function first

  if(button.isPressed()){
    pressedTime = millis();
    isPressing = true;
    isLongDetected = false;
  }

  if(button.isReleased()) {
    isPressing = false;
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    if( pressDuration < SHORT_PRESS_TIME )
      Serial.println("A short press is detected");
      configure = true; // Need to set a flag so we start the config routine
    }

  if(isPressing == true && isLongDetected == false) {
    long pressDuration = millis() - pressedTime;

    if( pressDuration > LONG_PRESS_TIME ) {
      Serial.println("A long press is detected");
      isLongDetected = true;
      // recordingStandAlone = true; // Let's record them waves...maybe start a delay
    }
  }
  
  
  if ((WiFi.status() == WL_CONNECTED || recordingStandAlone)) { // Testing
    
    if(recordingStandAlone || oscServerFound){
      // Serial.println("Read those values");
       startMillis = millis();             // Save the starting time

      #if defined Accelerometer
      if( checkAccel == true || checkPanTiltRoll == true ){
        if ( playback == 0 ) {
          error = readAccelerometer(); // Using the seed library (thank you!)  
          // Serial.println("doin' it");
        } else {

          aIntx = sensorData[sensorIndex];  // fill in the values from the sensorData array : " );
          //Serial.print("aIntx : " );
          //Serial.println(aIntx);
          aInty = sensorData[sensorIndex+1];
          //Serial.print("aInty : " );
          //Serial.println(aInty);
          aIntz = sensorData[sensorIndex+2];
          //Serial.print("aIntz : " );
          //Serial.println(aIntz);
          x = sensorData[sensorIndex+3];  
          y = sensorData[sensorIndex+4];
          z = sensorData[sensorIndex+5];
          
        }
      }
      #endif

      #if defined PressureSensor
      if( checkSuck == true || checkBlow == true ){
        if ( playback == 0 ) {
        error = readCFSensor(0x6D);         //start conversion and read on pressure sensor at 0x6D address
        
        
        } else {
          blowValue = sensorData[sensorIndex+6]; // fill in the values from the sensorData array here
          suckValue = sensorData[sensorIndex+7];
        }
      }
      #endif

      if ( checkSensor1 == true | checkSensor2 == true | checkSensor3 == true | checkSensor4 == true ){
        if  ( playback == 0 ) {
          // check the sensors here
          
          sensor1Value = analogRead(32);
          sensor1Value = map(sensor1Value, 360, 4094, 0, 127);
          Serial.print("sensor1Value : "); // 360 - 4095
          Serial.println(sensor1Value);
          sensor2Value = analogRead(33); // 0 - 3025
          sensor2Value = map(sensor2Value, 0, 3025, 0, 127);
          Serial.print("sensor2Value : ");
          Serial.println(sensor2Value);
          sensor3Value = analogRead(34);
          sensor3Value = map(sensor3Value, 740, 4010, 0, 127);
          Serial.print("sensor3Value : "); // 740 - 4010
          Serial.println(sensor3Value);
          sensor4Value = analogRead(35);
          sensor4Value = map(sensor4Value, 90, 4095, 0, 127);
          Serial.print("sensor4Value : ");
          Serial.println(sensor4Value); // 90 - 4095

          if (recording || recordingStandAlone) {
            sensorData[sensorIndex+8] = sensor1Value;
            sensorData[sensorIndex+9] = sensor2Value;
            sensorData[sensorIndex+10] = sensor3Value;
            sensorData[sensorIndex+11] = sensor4Value;
          }
          
        } else {
          sensor1Value = sensorData[sensorIndex+8]; // Read the values from the array
          sensor2Value = sensorData[sensorIndex+9];
          sensor3Value = sensorData[sensorIndex+10];
          sensor4Value = sensorData[sensorIndex+11];
        }   
      }
        
      
      while ((millis() - startMillis) < period);            //waits until period done
     
    } else if(!oscServerFound) {  
      //browseService("touchoscbridge", "udp");
      browseService("osc", "udp"); // _osc._udp
      // browseService("http", "udp"); // for another ESP32 running mDNS_Web_Server (udp)
      // browseService("http", "tcp"); // for another ESP32 running mDNS_Web_Server (tcp) 
    } 
     

    ////////// TFT Display values ///////////
    display.clearDisplay();
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(0, 48);
    
    if ( recordingStandAlone == 1 ) {
      display.print(F("Stand Alone Recording!"));
      }
      
    if ( recording == 1 ) {
      display.print(F("Recording This Party!"));
      }
    else if ( playback == 1 ){
      display.print(F("Playback Party Mode!")); 
      }
    else {
      display.print(F("Let's get this party started!"));// Start at top-left corner
    }

    // display.print(F("IP: "));// Start at top-left corner
    //display.println(WiFi.localIP());// Start at top-left corner
    //display.println(F("Client connected ?"));
   
    if (checkAccel){
      display.setCursor(0, 0);
      display.print(F("AX "));
      display.setCursor(16, 0);
      display.println(aIntx);
      
      midi[0] = storedAccelxChanInt;   // midi channel // 90 + midi channel (note on)
      midi[1] = aIntx; // SensorValue  // velocity
      midi[2] = storedAccelxCCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server

      display.setCursor(42, 0);
      display.print(F("AY "));
      display.setCursor(58, 0);
      display.println(aInty);

      midi[0] = storedAccelyChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = aInty; // SensorValue  // velocit
      midi[2] = storedAccelyCCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server

      display.setCursor(87, 0);
      display.print(F("AZ "));
      display.setCursor(104, 0);
      display.println(aIntz);

      midi[0] = storedAccelzChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = aIntz; // SensorValue  // velocit
      midi[2] = storedAccelzCCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server
    }

    if (checkPanTiltRoll){
      display.setCursor(0, 12);
      display.print(F("Pan "));
      display.setCursor(22, 12);
      display.println(x);

      midi[0] = storedPanChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = x; // SensorValue  // velocité
      midi[2] = storedPanCCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server

//      Serial.print("testing midi : ");
//      Serial.print(storedPanChanInt);
//      Serial.print(" ");
//      Serial.print(x);
//      Serial.print(" ");
//      Serial.print(storedPanCCInt);
//      Serial.println(" ");
      
      display.setCursor(41, 12);
      display.print(F("Tilt "));
      display.setCursor(67, 12);
      display.println(y);

      midi[0] = storedTiltChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = y; // SensorValue  // velocit
      midi[2] = storedTiltCCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server

      display.setCursor(88, 12);
      display.print(F("Roll "));
      display.setCursor(110, 12);
      display.println(z);

      midi[0] = storedRollChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = z; // SensorValue  // velocit
      midi[2] = storedRollCCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server
      
    }

    if (checkBlow){
      display.setCursor(0, 24);
      display.print(F("Blow: "));
      display.setCursor(28, 24);
      display.println(int(blowValue));   

      midi[0] = storedBlowChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = blowValue; // SensorValue  // velocity
      midi[2] = storedBlowCCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server
    }

    if (checkSuck){
      display.setCursor(64, 24);
      display.print(F("Suck:"));
      display.setCursor(94, 24);
      display.println(int(suckValue));  

      midi[0] = storedSuckChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = suckValue; // SensorValue  // velocity
      midi[2] = storedSuckCCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server
    }

    if (checkSensor1){
      display.setCursor(0, 36);
      display.print(F("S1:"));
      display.setCursor(16, 36);
      display.println(sensor1Value);  

      midi[0] = storedSensor1ChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = sensor1Value; // SensorValue  // velocity
      midi[2] = storedSensor1CCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server
    }

    if (checkSensor2){
      display.setCursor(32, 36);
      display.print(F("S2:"));
      display.setCursor(48, 36);
      display.println(sensor2Value);
       
      midi[0] = storedSensor2ChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = sensor2Value; // SensorValue  // velocit
      midi[2] = storedSensor2CCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server
    }

    if (checkSensor3){
      display.setCursor(64, 36);
      display.print(F("S3:"));
      display.setCursor(80, 36);
      display.println(sensor3Value);  

      midi[0] = storedSensor3ChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = sensor3Value; // SensorValue  // velocit
      midi[2] = storedSensor3CCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server
    }

    if (checkSensor4){
      display.setCursor(96, 36);
      display.print(F("S4:"));
      display.setCursor(112, 36);
      display.println(sensor4Value); 

      midi[0] = storedSensor4ChanInt;      // midi channel // 90 + midi channel (note on)
      midi[1] = sensor4Value; // SensorValue  // velocit
      midi[2] = storedSensor4CCInt;     // Control Change message (11 is expression) // Pitch (note value)
      midi[3] = 0;     // Extra                                          
      oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server
    }
   
    display.display();
    ////////////////////////////////

  /// Recording /// Get the values in an array !  Get ax, ay, az, x, y, z, blow, suck, s1, s2, s3, s4 into that thing

  if ( recording ) {

    for ( int i = 0; i <12; i++ ) { 
      switch(i){
        case 0: sensorData[i+sensorIndex] = aIntx; break; 
        case 1: sensorData[i+sensorIndex] = aInty; break;
        case 2: sensorData[i+sensorIndex] = aIntz; break;
        case 3: sensorData[i+sensorIndex] = x; break;
        case 4: sensorData[i+sensorIndex] = y; break;
        case 5: sensorData[i+sensorIndex] = z; break;
        case 6: sensorData[i+sensorIndex] = blowValue; break;
        case 7: sensorData[i+sensorIndex] = suckValue; break;
        case 8: sensorData[i+sensorIndex] = sensor1Value; break;
        case 9: sensorData[i+sensorIndex] = sensor2Value; break;
        case 10: sensorData[i+sensorIndex] = sensor3Value; break;
        case 11: sensorData[i+sensorIndex] = sensor4Value; break;
        default: break;
      } 
    }
   
  } // End if recording  
  ////////////////////////////

    analogWrite(LED_BUILTIN, ax*2); 

  
    sensorIndex = sensorIndex+12;
      
  }

  if (testWifi() == true && configure == 0) {
    return;
  } else {
    if (debugSerial) {
      Serial.println("Connection Status Negative / D0 HIGH");
      Serial.println("Turning the HotSpot On");
    }

    display.clearDisplay();
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(0, 0);
    display.print(F("Connect to : "));// Start at top-left corner
    display.setCursor(0, 10);
    display.print(monAP);// Start at top-left corner
    display.setCursor(0, 20);
    display.print(F("http//192.168.4.1"));// Start at top-left corner
    display.setCursor(5, 32);
    display.setTextSize(2);             // Draw 2X-scale text
    display.setTextColor(WHITE);
    // display.invertDisplay(true);
    display.println(F("Configure"));
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setCursor(0, 48);
    display.print(F("Dial up!"));
    display.display();
    
    launchWeb();
    setupAP();  // Setup HotSpot
      
  }
  
   // Serial.println("we are there");
  while ((WiFi.status() != WL_CONNECTED))
  { 
    Serial.println("**");
    analogWrite(LED_BUILTIN, 0);
    delay(125);
    analogWrite(LED_BUILTIN, 255);
    delay(125);
    server.handleClient();
    
   if (recordingStandAlone){
     break;
   }
    
  }

} // Fin du loop

void browseService(const char * service, const char * proto){
    if (debugSerial) {
      Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
    }
    int n = MDNS.queryService(service, proto);
    if (n == 0) {
      analogWrite(LED_BUILTIN, 255);
      delay(250);
      analogWrite(LED_BUILTIN, 0);
      delay(250);
      if (debugSerial) {
        Serial.println("no services found");
      }
    } else {
      
        if (debugSerial) {
          Serial.print(n);
          Serial.println(" service(s) found");
        }
        
        for (int i = 0; i < n; ++i) {
            oscUdp.setDestination(MDNS.IP(i), MDNS.port(i));
            if (debugSerial) {
              // Print details for each service found
              Serial.print("  ");
              Serial.print(i + 1);
              Serial.print(": ");
              Serial.print(MDNS.hostname(i));
              Serial.print(" (");
              Serial.print(MDNS.IP(i));
              Serial.print(":");
              Serial.print(MDNS.port(i));
              Serial.println(")");
            }
         }
         analogWrite(LED_BUILTIN, 255);
         oscServerFound = true;
         
      }
}


// Blow Suck sensor XGZP6897D I2C
bool readCFSensor(byte sensorAddress) {

  byte reg0xA5 = 0;

  Wire.beginTransmission(sensorAddress);    //send Start and sensor address
  Wire.write(0xA5);                         //send 0xA5 register address
  Wire.endTransmission();                   //send Stop
  Wire.requestFrom(sensorAddress, byte(1)); //send Start and read 1 byte command from sensor address
  if (Wire.available()) {                   //check if data is available on i2c buffer
    reg0xA5 = Wire.read();                  //read 0xA5 register value
  }
  Wire.endTransmission();                   //send Stop


  reg0xA5 = reg0xA5 & 0xFD;                 //mask 0xA5 register AND 0xFD to set ADC output calibrated data
  Wire.beginTransmission(sensorAddress);    //send Start and sensor address
  Wire.write(0xA5);                         //send 0xA5 register address
  Wire.write(reg0xA5);                      //send 0xA5 regiter new value
  Wire.endTransmission();                   //send Stop

  Wire.beginTransmission(sensorAddress);    //send Start and sensor address
  Wire.write(0x30);                         //send 0x30 register address
  Wire.write(0x0A);                         //set and start (0X0A = temperature + pressure, 0x01 just pressure)
  Wire.endTransmission();                   //send Stop

  byte reg0x30 = 0x30;                      //declare byte variable for 0x30 register copy (0x08 initializing for while enter)
  while ((reg0x30 & 0x08) > 0) {            //loop while bit 3 of 0x30 register copy is 1
    delay(1);                               //1mS delay
    Wire.beginTransmission(sensorAddress);  //send Start and sensor address
    Wire.write(0x30);                       //send 0x30 register address
    Wire.endTransmission();                 //send Stop
    Wire.requestFrom(sensorAddress, byte(1)); //send Start and read 1 byte command from sensor address
    if (Wire.available()) {                 //check if data is available on i2c buffer
      reg0x30 = Wire.read();                //read 0x30 register value
    }
    Wire.endTransmission();    //send Stop
  }

  unsigned long pressure24bit;              //declare 32bit variable for pressure ADC 24bit value
  byte pressHigh = 0;                       //declare byte temporal pressure high byte variable
  byte pressMid = 0;                        //declare byte temporal pressure middle byte variable
  byte pressLow = 0;                        //declare byte temporal pressure low byte variable

  Wire.beginTransmission(sensorAddress);    //send Start and sensor address
  Wire.write(0x06);                         //send pressure high byte register address
  Wire.endTransmission();                   //send Stop
  Wire.requestFrom(sensorAddress, byte(3)); //send Start and read 1 byte command from sensor address

  while (Wire.available() < 3);             //wait for 3 byte on buffer
  pressHigh = Wire.read();                  //read pressure high byte
  pressMid = Wire.read();                   //read pressure middle byte
  pressLow = Wire.read();                   //read pressure low byte
  Wire.endTransmission();                   //send Stop

  pressure24bit = pressure24bit | pressHigh;
  pressure24bit = pressure24bit & 0x000000FF;
  pressure24bit = pressure24bit << 8;

  pressure24bit = pressure24bit | pressMid;
  pressure24bit = pressure24bit & 0x0000FFFF;
  pressure24bit = pressure24bit << 8;

  pressure24bit = pressure24bit | pressLow;
  pressure24bit = pressure24bit & 0x00FFFFFF;


  if (pressure24bit > 8388608) {                                        //check sign bit for two's complement
    press = (float(pressure24bit) - float(16777216)) * 0.0000078125;    //KPa negative pressure calculation
  }
  else {                                                                //no sign
    press = float(pressure24bit) * 0.0000078125;                        //KPa positive pressure calculation
  }

  if(!(press >= 65.53) && (press >= 0.48 || press <= 0.34)) { // no calibration, just by observation when not touching the sensor
    
    if(press <= 0.34) { //weird when going full blast it jumps to 65.54 (from -)
        suckValue = map(press, -65.53, 0.34, 0, 127);

        if (debugSerial) {
          Serial.print("suckValue : ");
          Serial.println(suckValue);
        }

//        uint8_t midi[4];
//        midi[0] = storedSuckChanInt;      // midi channel // 90 + midi channel (note on)
//        midi[1] = suckValue; // SensorValue  // velocity
//        midi[2] = storedSuckCCInt;     // Control Change message (11 is expression) // Pitch (note value)
//        midi[3] = ;     // Extra                                     
//        
//        oscUdp.sendMessage("/midi",  "m",  midi); // send to Udp server
        //oscUdp.sendMessage("/tdlf",  "I", ccSuck); // send to Udp server  // T I 
        //oscUdp.sendMessage("/tdlf",  "i", ccSuck); // send to Udp server  // ok : T I i


    } else if(press >= 0.48) {
        blowValue = map(press, 0.48, 65.53, 0, 127);

          // Serial.print("blowValue : ");
          // Serial.println(blowValue);  

//        uint8_t midi[4];
//        midi[0] = storedBlowChanInt;
//        midi[1] = blowValue;
//        midi[2] = storedBlowCCInt;; // breath controller
//        midi[3] = 176;
//        oscUdp.sendMessage("/midi",  "m",  midi);
        
    }
    
  }

  if (recording || recordingStandAlone){
    
    sensorData[sensorIndex+6] = suckValue;
    sensorData[sensorIndex+7] = blowValue;
    
  }
  
  return 0;
}

bool readAccelerometer() {
  adxl.readXYZ(&x, &y, &z); //read the accelerometer values and store them in variables  x,y,z
  x = map(x,-320,320,0,127); // Should be 355 degrees but in practice I don't see values above 320...
  y = map(y,-320,320,0,127);
  z = map(z,-320,320,0,127);

  if (recording || recordingStandAlone){
    Serial.print("sensorIndex : ");
    Serial.println(sensorIndex);
    sensorData[sensorIndex] = x;
    sensorData[sensorIndex+1] = y;
    sensorData[sensorIndex+2] = z;
  }

  if (debugSerial) {
    // Output x,y,z values 
    Serial.print("values of X , Y , Z: ");
    Serial.print(x);
    Serial.print(" , ");
    Serial.print(y);
    Serial.print(" , ");
    Serial.println(z);
    }
  
  double xyz[3];
  //double ax,ay,az;
  adxl.getAcceleration(xyz);
  aIntx = int(xyz[0] * 100);
  aInty = int(xyz[1] * 100);
  aIntz = int(xyz[2] * 100);
  aIntx = map(aIntx,-200,200,0,127);
  aInty = map(aInty,-200,200,0,127);
  aIntz = map(aIntz,-200,200,0,127);

  if (recording || recordingStandAlone){
    //Serial.print("sensorIndex : ");
    //Serial.println(sensorIndex);
    sensorData[sensorIndex+3] = aIntx;
    sensorData[sensorIndex+4] = aInty;
    sensorData[sensorIndex+5] = aIntz;
  }

  if (debugSerial) {
//    Serial.print("X=");
//    Serial.print(aIntx);
//    Serial.println(" g");
//    Serial.print("Y=");
//    Serial.print(aInty);
//    Serial.println(" g");
//    Serial.print("Z=");
//    Serial.print(aIntz);
//    Serial.println(" g");
//    Serial.println("**********");
    delay(1); // 50
    }
 
  return 0;
} // End readAccelerometer


bool testWifi(void)
{
  int c = 0;
  //Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED || recordingStandAlone)
    {
      return true;
      Serial.println("hello standalone");
    }
    delay(500);
    if (debugSerial) {
      Serial.print("*");
    }
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}


void launchWeb()
{
  if (debugSerial) {
    Serial.println("");
  }
  if (WiFi.status() == WL_CONNECTED)

    if (debugSerial) {
      Serial.println("WiFi connected");
      Serial.print("Local IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("SoftAP IP: ");
      Serial.println(WiFi.softAPIP());
    }
  createWebServer();
  // Start the server
  server.begin();
  if (debugSerial) {
    Serial.println("Server started");
  }
}


void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  if (debugSerial) {
    Serial.println("scan done");
  }
  if (n == 0)
    if (debugSerial) {
      Serial.println("no networks found");
    }
    else
    {
      if (debugSerial) {
        Serial.print(n);
        Serial.println(" networks found");
      }
      for (int i = 0; i < 5; ++i) 
      // for (int i = 0; i < n; ++i) // changing this so only the first 5 APs get shown so we have space for other stuff.
      {
        // Print SSID and RSSI for each network found
        if (debugSerial) {
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(WiFi.SSID(i));
          Serial.print(" (");
          Serial.print(WiFi.RSSI(i));
          Serial.print(")");
        }
        //Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
        delay(10);
      }
    }
  if (debugSerial) {
    Serial.println("");
  }
  st = "<ol>";
  for (int i = 0; i < 5; ++i)
  // for (int i = 0; i < n; ++i)  // changing this so we only have 5 results
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    //st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP(monAP, "");
  if (debugSerial) {
    Serial.println("Initializing_softap_for_wifi credentials_modification");
  }
  launchWeb();
  if (debugSerial) {
    Serial.println("over");
  }
}


void createWebServer()
{
  {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><center><b> Cajita Abierta Configuration</b></center><br>";
      content += "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">";
      content += "<center><b>Wifi Credentials</b></center><br>";
      
      // menu
      content += "<b><center>";
      content += " <a href = \"accel.html\"> Accelerometer</a>";
      content += " / <a href = \"blowsuck.html\"> Blow suck</a>";
      content += " / <a href = \"sensors.html\"> Sensors</a><p>";
      content += " / <a href = \"record.html\"> Recording/Playback</a><p>";
      content += "</b></center>";
      
      content += "<style>" "body {  font-family: Arial, Helvetica, sans-serif;Color: #111111;font-size: 18px; margin-left:auto;margin-right:auto;width:400px; }";
      content += "ol {list-style-type: none}";
      content += "</style>";
      content += "<ol><li>";
      content += "<form action=\"/scan\" method=\"POST\">";
      content += "<INPUT type=\"submit\" value=\" scan\" style=\"font-size:18px ; background-color:#DD622D ; border: none;\">";
      content += "</form>";
      content += "<p>"; 
      content += "</li>";
      content += st;
      content += "<li>";
      content += "</p><form method='get' action='wificfg'><label> SSID: </label><input name='ssid' style=\"font-size:18px;\" required size=20><br /><label> PWD: </label><input name='pass' type='password' style=\"font-size:18px;\" size=20> <p>";
      content += "<INPUT type=\"submit\" value=\"submit\" style=\"font-size:18px ; background-color:#DD622D ; border: none;\">";
      content += "</form>";
      content += "</li><li>";
      content += "<form method='get' action='reboot'>";
      content += "<INPUT type=\"submit\" value=\"reboot\" style=\"font-size:18px ; background-color:#DD622D ; border: none;\">";
      content += "</form>";
      content += "</li><li>";
    
      content += "</html>";
      server.send(200, "text/html", content);
    });

    /////// Accelerometer ////////
    server.on("/accel.html", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><center><b>Cajita Abierta Configuration</b></center><br>";
      content += "<center><b>Accelerometer</b></center><br>";
      content += "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">";
      content += "<b><center>";
      content += " <a href = \"/\"> WiFI </a>";
      content += " / <a href = \"blowsuck.html\"> Blow suck </a>";
      content += " / <a href = \"sensors.html\"> Sensors </a>";
      content += "</b></center>";
      
      content += "<style>" "body {  font-family: Arial, Helvetica, sans-serif;Color: #111111;font-size: 18px; margin-left:auto;margin-right:auto;width:400px; }";
      content += "ol {list-style-type: none}";
      content += "</style>";
      content += "<p>";

      content += "</p><form method='get' action='accelcfg'>";

      content += "<ol><li>";
      content += " Accel X <label for=\"accelxChan\">Channel</label><select name=\"accelxChan\" id=\"accelxChan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"accelxCC\">CC</label><select name=\"accelxCC\" id=\"accelxCC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li><li>";
      content += " Accel Y <label for=\"accelyChan\">Channel</label><select name=\"accelyChan\" id=\"accelyChan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"accelyCC\">CC</label><select name=\"accelyCC\" id=\"accelyCC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li><li>";
      content += " Accel Z <label for=\"accelzChan\">Channel</label><select name=\"accelzChan\" id=\"accelzChan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"accelzCC\">CC</label><select name=\"accelzCC\" id=\"accelzCC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li></ol>";

      content += "<ol><li>";
      content += "Pan <label for=\"panChan\"> Channel</label><select name=\"panChan\" id=\"panChan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"panCC\">CC</label><select name=\"panCC\" id=\"panCC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li><li>"; 
      content += "Tilt <label for=\"tiltChan\">Channel</label><select name=\"tiltChan\" id=\"tiltChan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"tilt\">tiltCC</label><select name=\"tiltCC\" id=\"tiltCC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li><li>"; 
      content += "Roll <label for=\"rollChan\"> Channel</label><select name=\"rollChan\" id=\"rollChan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"Roll\">rollCC</label><select name=\"rollCC\" id=\"rollCC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li></ol>";
      //content += "<p>"; 
      
      content += "<ol><li>";
      content += "<label for='stx'>NIY!! Single tap X </label><input type='checkbox' id='stx' name='stx' value='enabled'>";
      content += "<label for='dtx'>NIY!!  Double tap X </label><input type='checkbox' id='dtx' name='dtx' value='enabled'>";
      content += "</li><li>";
      content += "<label for='sty'>NIY!! Single tap Y </label><input type='checkbox' id='sty' name='sty' value='enabled'>";
      content += "<label for='dty'>NIY!!  Double tap Y </label><input type='checkbox' id='dty' name='dty' value='enabled'>";
      content += "</li><li>";
      content += "<label for='stz'>NIY!! Single tap Z </label><input type='checkbox' id='stz' name='stz' value='enabled'>";
      content += "<label for='dtz'>NIY!! Double tap Z </label><input type='checkbox' id='dtz' name='dtz' value='enabled'>";
      content += "</li><li>";
      content += "<label for='freefall'>NIY!! Freefall </label><input type='checkbox' id='freefall' name='freefall' value='enabled'><br>";
      content += "</li></ol>";
      
      content += "<INPUT type=\"submit\" value=\"save\" style=\"font-size:18px ; background-color:#DD622D ; border: none;\">";
      content += "</form>";
      content += "</html>";
      server.send(200, "text/html", content);    
    });

    //// Fin Config Accelerometer

    /////// Blow suck ////////
    server.on("/blowsuck.html", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><center><b>Cajita Abierta Configuration</b></center><br>";
      content += "<center><b>Blow / Suck</b></center><br>";
      content += "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">";
      content += "<b><center>";
      content += " <a href = \"/\"> WiFI </a>";
      content += " / <a href = \"accel.html\"> Accelerometer </a>";
      content += " / <a href = \"sensors.html\"> Sensors </a>";
      content += "</b></center>";
      
      content += "<style>" "body {  font-family: Arial, Helvetica, sans-serif;Color: #111111;font-size: 18px; margin-left:auto;margin-right:auto;width:400px; }";
      content += "ol {list-style-type: none}";
      content += "</style>";
      content += "<p>";

      content += "</p><form method='get' action='blowsuckcfg'>";

      content += "<ol><li>";
      content += " Blow <label for=\"blowChan\">Channel</label><select name=\"blowChan\" id=\"blowChan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"blowCC\">CC</label><select name=\"blowCC\" id=\"blowCC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li><li>";
      content += " Suck <label for=\"suckChan\">Channel</label><select name=\"suckChan\" id=\"suckChan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"suckCC\">CC</label><select name=\"suckCC\" id=\"suckCC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li></ol>";

      content += "<INPUT type=\"submit\" value=\"save\" style=\"font-size:18px ; background-color:#DD622D ; border: none;\">";
      content += "</form>";
      content += "</html>";
      server.send(200, "text/html", content);    
    });

    /// fin config Blow suck

    //// Sensors ////
    
    server.on("/sensors.html", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><center><b>Cajita Abierta Configuration</b></center><br>";
      content += "<center><b>Sensors</b></center><br>";
      content += "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">";
      content += "<b><center>";
      content += " <a href = \"/\"> WiFI </a>";
      content += " / <a href = \"accel.html\"> Accelerometer </a>";
      content += " / <a href = \"blowsuck.html\"> Blow suck </a>";
      content += "</b></center>";
      
      content += "<style>" "body {  font-family: Arial, Helvetica, sans-serif;Color: #111111;font-size: 18px; margin-left:auto;margin-right:auto;width:400px; }";
      content += "ol {list-style-type: none}";
      content += "</style>";
      content += "<p>";

      content += "</p><form method='get' action='sensorscfg'>";
      
      content += "<ol><li>";
      content += " Sensor 1 <label for=\"sensor1Chan\">Channel</label><select name=\"sensor1Chan\" id=\"sensor1Chan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"sensor1CC\">CC</label><select name=\"sensor1CC\" id=\"sensor1CC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li><li>";
      content += " Sensor 2 <label for=\"sensor2Chan\">Channel</label><select name=\"sensor2Chan\" id=\"sensor2Chan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"sensor2CC\">CC</label><select name=\"sensor2CC\" id=\"sensor2CC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li><li>";
      content += " Sensor 3 <label for=\"sensor3Chan\">Channel</label><select name=\"sensor3Chan\" id=\"sensor3Chan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"sensor3CC\">CC</label><select name=\"sensor3CC\" id=\"sensor3CC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li><li>";
      content += " Sensor 4 <label for=\"sensor4Chan\">Channel</label><select name=\"sensor4Chan\" id=\"sensor4Chan\" style=\"font-size:18px;\"><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option></select>";
      content += "<label for=\"sensor4CC\">CC</label><select name=\"sensor4CC\" id=\"sensor4CC\" style=\"font-size:18px;\" ><option value=\"0\">0</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option><option value=\"15\">15</option><option value=\"16\">16</option><option value=\"17\">17</option><option value=\"18\">18</option><option value=\"19\">19</option><option value=\"20\">20</option><option value=\"21\">21</option><option value=\"22\">22</option><option value=\"23\">23</option><option value=\"24\">24</option><option value=\"25\">25</option><option value=\"26\">26</option><option value=\"27\">27</option><option value=\"28\">28</option><option value=\"29\">29</option><option value=\"30\">30</option><option value=\"31\">31</option><option value=\"32\">32</option><option value=\"33\">33</option><option value=\"34\">34</option><option value=\"35\">35</option><option value=\"36\">36</option><option value=\"37\">37</option><option value=\"38\">38</option><option value=\"39\">39</option><option value=\"40\">40</option><option value=\"41\">41</option><option value=\"42\">42</option><option value=\"43\">43</option><option value=\"44\">44</option><option value=\"45\">45</option><option value=\"46\">46</option><option value=\"47\">47</option><option value=\"48\">48</option><option value=\"49\">49</option><option value=\"50\">50</option><option value=\"51\">51</option><option value=\"52\">52</option><option value=\"53\">53</option><option value=\"54\">54</option><option value=\"55\">55</option><option value=\"56\">56</option><option value=\"57\">57</option><option value=\"58\">58</option><option value=\"59\">59</option><option value=\"60\">60</option><option value=\"61\">61</option><option value=\"62\">62</option><option value=\"63\">63</option><option value=\"64\">64</option><option value=\"65\">65</option><option value=\"66\">66</option><option value=\"67\">67</option><option value=\"68\">68</option><option value=\"69\">69</option><option value=\"70\">70</option><option value=\"71\">71</option><option value=\"72\">72</option><option value=\"73\">73</option><option value=\"74\">74</option><option value=\"75\">75</option><option value=\"76\">76</option><option value=\"77\">77</option><option value=\"78\">78</option><option value=\"79\">79</option><option value=\"80\">80</option><option value=\"81\">81</option><option value=\"82\">82</option><option value=\"83\">83</option><option value=\"84\">84</option><option value=\"85\">85</option><option value=\"86\">86</option><option value=\"87\">87</option><option value=\"88\">88</option><option value=\"89\">89</option><option value=\"90\">90</option><option value=\"91\">91</option><option value=\"92\">92</option><option value=\"93\">93</option><option value=\"94\">94</option><option value=\"95\">95</option><option value=\"96\">96</option><option value=\"97\">97</option><option value=\"98\">98</option><option value=\"99\">99</option><option value=\"100\">100</option><option value=\"101\">101</option><option value=\"102\">102</option><option value=\"103\">103</option><option value=\"104\">104</option><option value=\"105\">105</option><option value=\"106\">106</option><option value=\"107\">107</option><option value=\"108\">108</option><option value=\"109\">109</option><option value=\"110\">110</option><option value=\"111\">111</option><option value=\"112\">112</option><option value=\"113\">113</option><option value=\"114\">114</option><option value=\"115\">115</option><option value=\"116\">116</option><option value=\"117\">117</option><option value=\"118\">118</option><option value=\"119\">119</option><option value=\"120\">120</option><option value=\"121\">121</option><option value=\"122\">122</option><option value=\"123\">123</option><option value=\"124\">124</option><option value=\"125\">125</option><option value=\"126\">126</option><option value=\"127\">127</option></select>";
      content += "</li></ol>";
     
      content += "<INPUT type=\"submit\" value=\"save\" style=\"font-size:18px ; background-color:#DD622D ; border: none;\">";
      content += "</form>";
      content += "</html>";
      server.send(200, "text/html", content);    
    });

    ///// Fin sensors ////

    ///// Recording / Playback /////
    
    server.on("/record.html", []() {
      
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><center><b>Cajita Abierta Configuration</b></center><br>";
      content += "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">";
      content += "<b><center>";
      content += " <a href = \"/\"> WiFI </a>";
      content += " / <a href = \"accel.html\"> Accelerometer </a>";
      content += " / <a href = \"blowsuck.html\"> Blow suck </a>";
      content += " / <a href = \"sensors.html\"> Sensors </a>";
      content += "</b></center>";
      content += "<center><b>Recording / Playback</b></center><br>";
      
      content += "<style>" "body {  font-family: Arial, Helvetica, sans-serif;Color: #111111;font-size: 18px; margin-left:auto;margin-right:auto;width:400px; }";
      content += "ol {list-style-type: none}";
      content += "</style>";
      content += "<p>";

      content += "<ol><li>";
  
      content += "</p><p><form method='get' action='recordcfg'>";
      // content += "<label> File : </label><input name='filename' style=\"font-size:18px;\" required length=24>";
      content += "<INPUT type=\"submit\" value=\"record\" style=\"font-size:20px ; background-color:#DD622D ; border: none;\">";
      content += "</form>";
      content += "</li><li>";

      content += "</p><p><form method='get' action='recordingStandAlonecfg'>";
      // content += "<label> File : </label><input name='filename' style=\"font-size:18px;\" required length=24>";
      content += "<INPUT type=\"submit\" value=\"record no WiFI\" style=\"font-size:20px ; background-color:#DD622D ; border: none;\">";
      content += "<label for=\"Delay\">Delay</label><select name=\"monDelai\" id=\"monDelai\" style=\"font-size:18px;\" > <option value=\"5\">5</option><option value=\"10\">10</option><option value=\"20\">20</option><option value=\"30\">30</option><option value=\"40\">40</option><option value=\"50\">50</option><option value=\"60\">60</option><option value=\"300\">300</option></select>";
      content += "</form>";
      content += "</li><li>";

      
      content += "</p><p><form method='get' action='savecfg'>";
      content += "<INPUT type=\"submit\" value=\"save\" style=\"font-size:20px ; background-color:#FF622D ; border: none;\">";
      content += "</form>";
      content += "</li><li>";

      content += "</p><p><form method='get' action='playcfg'>";
      content += "<INPUT type=\"submit\" value=\"playback\" style=\"font-size:20px ; background-color:#DD622D ; border: none;\">";
      // content += "<br>";
      // content += "<label> File : </label><input name='filename' style=\"font-size:18px;\" required length=24>";
      // content += "<label> BPM : </label><input type ='text' name='bpm' style=\"font-size:20px;\" size=4>";
      // need to add options for playback such as file name, speed or smoothing
      content += "</form>";
      content += "</li><li>";

      content += "</p><p><form method='get' action='erasecfg'>";
      content += "<INPUT type=\"submit\" value=\"erase files\" style=\"font-size:20px ; background-color:#DD622D ; border: none;\">";
      // currently will reformat SPIFFS, need to add option to remove selected files
      content += "</p></form>";
      
      content += "</li></ol>";
     
      content += "</html>";
      server.send(200, "text/html", content);    
    });

      ///// Fin recording /////

    server.on("/accelcfg", []() {
      
      String accelxChan = server.arg("accelxChan");
      String accelyChan = server.arg("accelyChan");
      String accelzChan = server.arg("accelzChan");
      
      String accelxCC = server.arg("accelxCC");
      String accelyCC = server.arg("accelyCC");
      String accelzCC = server.arg("accelzCC");

      String accelPanChan = server.arg("panChan");
      String accelTiltChan = server.arg("tiltChan");
      String accelRollChan = server.arg("rollChan");
      
      String accelPanCC = server.arg("panCC");
      String accelTiltCC = server.arg("tiltCC");
      String accelRollCC = server.arg("rollCC");

      Serial.print("accelxChan : ");
      Serial.print(accelxChan);
      Serial.print(" accelxCC : ");
      Serial.println(accelxCC);

      Serial.print("accelyChan : ");
      Serial.print(accelyChan);
      Serial.print(" accelyCC : ");
      Serial.println(accelyCC);

      Serial.print("accelzChan : ");
      Serial.print(accelzChan);
      Serial.print(" accelzCC : ");
      Serial.println(accelzCC);

      Serial.print("accelPanChan : ");
      Serial.print(accelPanChan);
      Serial.print(" accelPanCC : ");
      Serial.println(accelPanCC);

      Serial.print("accelTiltChan : ");
      Serial.print(accelTiltChan);
      Serial.print(" acceTiltCC : ");
      Serial.println(accelTiltCC);

      Serial.print("accelRollChan : ");
      Serial.print(accelRollChan);
      Serial.print(" accelRollCC : ");
      Serial.println(accelRollCC);


      //// Writing to EEPROM ////
      if (debugSerial) {
          Serial.println("clearing eeprom");
        }
        for (int i = 64; i < 256; ++i) { // Find the char bounds of this section (13 x 2 + 13 x 3) = 65 chars
          EEPROM.write(i, 0);
        }
      if (debugSerial) {
        Serial.println("Writing accelxChan :");
        }
        for (int i = 0; i < accelxChan.length(); ++i)
        {
          EEPROM.write(64 + i, accelxChan[i]); // + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelxChan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing accelxCC  :"); // +3 
        }
        for (int i = 0; i < accelxCC.length(); ++i)
        {
          EEPROM.write(66 + i, accelxCC[i]); // 32 (esid) + 32 (password) +2 (accelxCC)
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelxCC[i]);
          }
        } //// end write accelx

        ////
    if (debugSerial) {
        Serial.println("Writing accelyChan :");
        }
        for (int i = 0; i < accelyChan.length(); ++i)
        {
          EEPROM.write(69 + i, accelyChan[i]); // next 69 + 2
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelyChan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing accelyCC  :"); //  
        }
        for (int i = 0; i < accelyCC.length(); ++i)
        {
          EEPROM.write(71 + i, accelyCC[i]); // 71 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelyCC[i]);
          }
        } //// end write accely
        
        ////

         if (debugSerial) {
        Serial.println("Writing accelzChan :");
        }
        for (int i = 0; i < accelzChan.length(); ++i)
        {
          EEPROM.write(74 + i, accelzChan[i]); // + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelzChan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing accelzCC  :"); // 
        }
        for (int i = 0; i < accelzCC.length(); ++i)
        {
          EEPROM.write(76 + i, accelzCC[i]); // next 76 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelzCC[i]);
          }
        } //// end write accelz

        //// Pan ////
      if (debugSerial) {
        Serial.println("Writing Pan Chan :");
        }
        for (int i = 0; i < accelPanChan.length(); ++i)
        {
          EEPROM.write(79 + i, accelPanChan[i]); // + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelPanChan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing Pan CC  :"); // 
        }
        for (int i = 0; i < accelPanCC.length(); ++i)
        {
          EEPROM.write(81 + i, accelPanCC[i]); // next 81 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelPanCC[i]);
          }
        } //// End write Pan

        /// Tilt ///
      if (debugSerial) {
        Serial.println("Writing Tilt Chan :");
        }
        for (int i = 0; i < accelTiltChan.length(); ++i)
        {
          EEPROM.write(84 + i, accelTiltChan[i]); // next 84 + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelTiltChan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing Tilt CC  :"); // 
        }
        for (int i = 0; i < accelTiltCC.length(); ++i)
        {
          EEPROM.write(86 + i, accelTiltCC[i]); // next 86 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelTiltCC[i]);
          }
        } /// End write Tilt ///

        /// Roll ///
      
      if (debugSerial) {
        Serial.println("Writing Roll Chan :");
        }
        for (int i = 0; i < accelRollChan.length(); ++i)
        {
          EEPROM.write(89 + i, accelRollChan[i]); // next 89 + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelRollChan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing Roll CC  :"); // 
        }
        for (int i = 0; i < accelRollCC.length(); ++i)
        {
          EEPROM.write(91 + i, accelRollCC[i]); // next 91 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(accelRollCC[i]);
          }
        } /// End write Roll ///

        // Add Taps, Doubletaps + freefall here //

        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi ... or hit back to continue with the config\"}";
        statusCode = 200;
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(statusCode, "application/json", content);
    }); // Fin accelcfg

    ///
    server.on("/blowsuckcfg", []() {
      
      String blowChan = server.arg("blowChan");
      String suckChan = server.arg("suckChan");
      String blowCC = server.arg("blowCC");
      String suckCC = server.arg("suckCC");

      Serial.print("blowChan : ");
      Serial.print(blowChan);
      Serial.print(" blowCC : ");
      Serial.println(blowCC);

      Serial.print("suckChan : ");
      Serial.print(suckChan);
      Serial.print(" suckCC : ");
      Serial.println(suckCC);

      //// Blow ////
      if (debugSerial) {
          Serial.println("clearing eeprom");
        }
        for (int i = 256; i < 266; ++i) { // Find the char bounds of this section (13 x 2 + 13 x 3) = 65 chars
          EEPROM.write(i, 0);
        }
      if (debugSerial) {
        Serial.println("Writing Blow Chan :");
        }
        for (int i = 0; i < blowChan.length(); ++i)
        {
          EEPROM.write(256 + i, blowChan[i]); // next 256 + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(blowChan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing blow CC  :"); // 
        }
        for (int i = 0; i < blowCC.length(); ++i)
        {
          EEPROM.write( 258 + i, blowCC[i]); // next 258 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(blowCC[i]);
          }
        } //// End write Blow

      //// Suck ////
      if (debugSerial) {
        Serial.println("Writing Suck Chan :");
        }
        for (int i = 0; i < suckChan.length(); ++i)
        {
          EEPROM.write(261 + i, suckChan[i]); // next 261 + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(suckChan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing Suck CC  :"); // 
        }
        for (int i = 0; i < suckCC.length(); ++i)
        {
          EEPROM.write( 263 + i, suckCC[i]); // next 263 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(suckCC[i]);
          }
        } //// End write Suck

      EEPROM.commit();
      content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi ... or hit back to continue with the config\"}";
      statusCode = 200;
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    }); // Fin blowsuckcfg

    server.on("/sensorscfg", []() {

      String sensor1Chan = server.arg("sensor1Chan");
      String sensor1CC = server.arg("sensor1CC");
      
      String sensor2Chan = server.arg("sensor2Chan");
      String sensor2CC = server.arg("sensor2CC");
      
      String sensor3Chan = server.arg("sensor3Chan");
      String sensor3CC = server.arg("sensor3CC");
      
      String sensor4Chan = server.arg("sensor4Chan");
      String sensor4CC = server.arg("sensor4CC");
  
      if (debugSerial) {
          Serial.println("clearing sensor info from eeprom ");
        }
        for (int i = 266; i < 286 ; ++i) {
          EEPROM.write(i, 0);
        }
      //// Sensor 1 ////
      if (debugSerial) {
        Serial.println("Writing Sensor1 Chan :");
        }
        for (int i = 0; i < sensor1Chan.length(); ++i)
        {
          EEPROM.write(266 + i, sensor1Chan[i]); // next 266 + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(sensor1Chan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing Sensor1 CC  :"); // 
        }
        for (int i = 0; i < sensor1CC.length(); ++i)
        {
          EEPROM.write( 268 + i, sensor1CC[i]); // next 268 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(sensor1CC[i]);
          }
        } //// End write sensor1

        //// Sensor 2 ////
      if (debugSerial) {
        Serial.println("Writing Sensor2 Chan :");
        }
        for (int i = 0; i < sensor2Chan.length(); ++i)
        {
          EEPROM.write(271 + i, sensor2Chan[i]); // next 271 + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(sensor2Chan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing Sensor2 CC  :"); // 
        }
        for (int i = 0; i < sensor2CC.length(); ++i)
        {
          EEPROM.write( 273 + i, sensor2CC[i]); // next 273 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(sensor2CC[i]);
          }
        } //// End write sensor2

        //// Sensor 3 ////
      if (debugSerial) {
        Serial.println("Writing Sensor3 Chan :");
        }
        for (int i = 0; i < sensor3Chan.length(); ++i)
        {
          EEPROM.write(276 + i, sensor3Chan[i]); // next 276 + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(sensor3Chan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing Sensor3 CC  :"); // 
        }
        for (int i = 0; i < sensor3CC.length(); ++i)
        {
          EEPROM.write( 278 + i, sensor3CC[i]); // next 278 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(sensor3CC[i]);
          }
        } //// End write sensor3

        //// Sensor 4 ////
      if (debugSerial) {
        Serial.println("Writing Sensor4 Chan :");
        }
        for (int i = 0; i < sensor4Chan.length(); ++i)
        {
          EEPROM.write(281 + i, sensor4Chan[i]); // next 281 + 2 
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(sensor4Chan[i]);
          }
        }
        ///// control change  ////
        if (debugSerial) {
          Serial.println("Writing Sensor4 CC  :"); // 
        }
        for (int i = 0; i < sensor4CC.length(); ++i)
        {
          EEPROM.write( 283 + i, sensor4CC[i]); // next 283 + 3
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(sensor4CC[i]);
          }
        } //// End write sensor4
       
       
      EEPROM.commit();
      content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi ... or hit back to continue with the config\"}";
      statusCode = 200;
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
       });
    


    /// Fin sensorscfg ///

    /// Record ///
    server.on("/recordcfg", []() {

      content = "{\"Success\":\"Starting recording ... hit back to continue with the config\"}";
      statusCode = 200;
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);

      //delay(4000); // Start recording in 4s...
      recording = true;

      if ( debugSerial ) {
      Serial.println("Recording started");
        }

      // End the web configure
      server.stop();
      server.close();

      // Close the access point
      WiFi.softAPdisconnect(true);  // Event we should get : SYSTEM_EVENT_AP_STOP
      WiFi.disconnect(); 
      WiFi.begin(esid.c_str(), epass.c_str());
      
      if (debugSerial) {
        Serial.println("Server stopped and WiFi restarted");
        }

      display.clearDisplay();
      display.setTextSize(2);             // Normal 1:1 pixel scale
      display.setCursor(3, 5);
      display.print(F("Recording"));// Start at top-left corner
      display.setCursor(3, 25);
      display.print(F("Mode"));// Start at top-left corner
      display.display();
  
      configure = false; // reset that thang
      sensorIndex = 0; // Start recording anew
      playback = 0; // No time-warp
      recordingStandAlone = 0; // Mutually exclusive
      
         
    }); // Fin Record

    server.on("/recordingStandAlonecfg", []() {

      int monDelai = server.arg("monDelai").toInt();
      Serial.print("monDelai : ");
      Serial.println(monDelai);

      content = "{\"Success\":\"Starting recording Stand Alone ... hit back to continue with the config\"}";
      statusCode = 200;
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);

      // End the web configure
      server.stop();
      server.close();

      // Close the access point
      WiFi.softAPdisconnect(true);  // Event we should get : SYSTEM_EVENT_AP_STOP
      WiFi.disconnect(); 
      WiFi.begin(esid.c_str(), epass.c_str());
      
      if (debugSerial) {
        Serial.println("Server stopped and WiFi restarted");
        }
        
      display.clearDisplay();
      display.setTextSize(2);             // Normal 1:1 pixel scale
      display.setCursor(0, 5);
      display.print(F("Bouteille"));// Start at top-left corner
      display.setCursor(0, 25);
      display.print(F("a la mer"));// Start at top-left corner
      display.setCursor(0, 45);
      display.print(F("..."));// Start at top-left corner
      display.display();
      delay(3000);

      for ( int i = monDelai; i >= 1; i--){
        display.clearDisplay();
        display.setTextSize(2);             // Normal 1:1 pixel scale
        display.setCursor(55, 25);
        display.print((i));// Start at top-left corner
        display.display();
        delay(1000);
      }
      
      recordingStandAlone = true;
      Serial.println("Recording Stand Alone started");
      configure = false; // reset that thang
      sensorIndex = 0; // Start recording anew
      playback = 0; // Can't record and playback at the same time
      recording = 0; // Only one record mode at a time 
      
    }); // Fin RecordingStandAlone


    /// Play ///
    server.on("/playcfg", []() {

      Serial.println("Will open file, save it into the array and start going through the array");

      File filetoRead = LITTLEFS.open("/sensorData.txt", "r");
      
      if(!filetoRead || filetoRead.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
        }
        
      Serial.println("- read from file:");

      display.clearDisplay();
      display.setTextSize(2);             // Normal 1:1 pixel scale
      display.setCursor(3, 5);
      display.print(F("Loading"));// Start at top-left corner
      display.setCursor(3, 25);
      display.print(F("File"));// Start at top-left corner
      display.setCursor(0,48);
      display.setTextSize(1);  
      display.print(F("Might take time"));
      display.display();
      
      int count = 0;
      char buffer[6]; // 6
      int value;
      
      while(filetoRead.available()){
        int l = filetoRead.readBytesUntil('\n', buffer, sizeof(buffer)-1);
        buffer[l] = '\0';
        value = atoi(buffer);
        sensorData[count] = value; // Populating the sensorData array
        
        Serial.print("count : ");
        Serial.println(count);
//        Serial.print("value : ");
//        Serial.println(value);
//        Serial.print("sensorData[count] : ");
//        Serial.println(sensorData[count]);
        count++;
        
        if ( count > arrayLength ) {
          break;
          }
        }
        
        filetoRead.close();

      display.clearDisplay();
      display.setTextSize(2);             // Normal 1:1 pixel scale
      display.setCursor(3, 5);
      display.print(F("Playback"));// Start at top-left corner
      display.setCursor(3, 25);
      display.print(F("Mode"));// Start at top-left corner
      display.setCursor(0,48);
      display.setTextSize(1);  
      display.print(F("Re-play Party"));
      display.display();
      delay(4000);

      content = "{\"Success\":\"Starting playback in 4s ... hit back to continue with the config\"}";
      statusCode = 200;
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);

 
      

      // End the web configure
      server.stop();
      server.close();

      // Close the access point
      WiFi.softAPdisconnect(true);  // Event we should get : SYSTEM_EVENT_AP_STOP
      WiFi.disconnect(); 
      WiFi.begin(esid.c_str(), epass.c_str());
      
      if (debugSerial) {
        Serial.println("Server stopped and WiFi restarted");
        Serial.println("Start playback");
        }

      playback = true;
      configure = false; // reset that thang
      sensorIndex = 0; // Start playing from the top
      recording = 0; // Don't attempt to record data if we're reading back from the sensorData array.
      recordingStandAlone = 0; // Don't attempt to record data if we're reading back from the sensorData array.
      
    }); // Fin Play


    /// Save ///
    server.on("/savecfg", []() {

      // use the array and save it, line by line with commas between the data points
      Serial.println("Will save to file"); // save to file... appending I guess...

      display.clearDisplay();
      display.setTextSize(2);  // Normal 1:1 pixel scale
      display.setCursor(0, 5);
      display.println(F("Saving to"));// Start at top-left corner
      display.setCursor(0, 25); 
      display.println(F("File"));
      display.setCursor(0,48);
      display.setTextSize(1);  
      display.print(F("Might take a few mins"));
      display.display();
     

      File fileToAppend = LITTLEFS.open("/sensorData.txt", "a");       // open file
      
      for ( int i = 0 ; i < arrayLength; i++){ // Change this back to 65535
        
          fileToAppend.println(sensorData[i]);
          //delay(2);
          //Serial.print("sensorData[i] : "); 
          //Serial.println(sensorData[i]);
          //fileToAppend.println(i);
          //fileToAppend.print(',');
          //Serial.println("doing it");     
      }
      
      fileToAppend.close();

      display.clearDisplay();
      display.setTextSize(2);  // Normal 1:1 pixel scale
      display.setCursor(0, 5);
      display.println(F("File saved"));// Start at top-left corner
      display.setCursor(0,48);
      display.setTextSize(1);  
      display.print(F("After party reboot!"));
      display.display();
      delay(3000);

      ESP.restart(); // Rebooting

//      content = "{\"Success\":\"Saving file ... This might take time ... hit back to continue with the config\"}";
//      statusCode = 200;
//      server.sendHeader("Access-Control-Allow-Origin", "*");
//      server.send(statusCode, "application/json", content);
//
//      // End the web configure
//      server.stop();
//      server.close();
//
//      // Close the access point
//      WiFi.softAPdisconnect(true);  // Event we should get : SYSTEM_EVENT_AP_STOP
//      WiFi.disconnect(); 
//      WiFi.begin(esid.c_str(), epass.c_str());

//      playback = false;
//      configure = 0; // Go back to config mode once the file is saved
//      sensorIndex = 0; // Start playing from the top
//      recording = 0; // Don't attempt to record data if we're reading back from the sensorData array.
//      recordingStandAlone = 0; // Don't attempt to record data if we're reading back from the sensorData
//      
    }); // Fin Save

    /// Erase files ///
    server.on("/erasecfg", []() {

      //SPIFFS.remove("/sensor.txt"); // Currently only that one file...
      LITTLEFS.remove("/sensorData.txt"); // Currently only that one file...
      content = "{\"Success\":\"Erasing files ... hit back to continue with the config\"}";
      statusCode = 200;
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
      
    }); // Fin Erase Files

    server.on("/reboot", []() {
      content = "{\"Success\":\" Rebooting, let's get this party started\"}";
      statusCode = 200;
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
      delay(2000);
      ESP.restart();
    });

    
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });
    
    server.on("/wificfg", []() {  // Was 'settings'
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");

      // SID / PWD
      if (qsid.length() > 0) {
        if (debugSerial) {
          Serial.println("clearing eeprom");
        }
        for (int i = 0; i < 64; ++i) {
          EEPROM.write(i, 0);
        }
        if (debugSerial) {
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");        
          Serial.println("writing eeprom ssid:");
        }
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(qsid[i]);
          }
        }
        if (debugSerial) {
          Serial.println("writing eeprom pass:");
        }
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          if (debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(qpass[i]);
          }
        }

        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi ... go back to keep configuring sensors\"}";
        statusCode = 200;
        // ESP.restart();

      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        if (debugSerial) {
          Serial.println("Sending 404");
        }
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    });
  }
}
