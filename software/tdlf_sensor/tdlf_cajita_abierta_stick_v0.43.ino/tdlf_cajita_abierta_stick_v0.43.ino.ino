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

#include "config.h"
#include "display.h"
#include "button.h"

//// Sensors ////
const int arrayLength = 10000; // 65535
byte sensorData[arrayLength] = {0}; // Array to store the values, intialize with 0s
long sensorIndex = 0; // Keep track of the values in the array
const long period = 5; //time between samples in milliseconds // change from '10' to '5' when not printing to serial

#include "accelerometer.h"
#include "blowsuck.h"
#include "analogSensors.h"
#include "params.h"
#include "portal.h"
//bool readCFSensor(byte sensorAddress);
//bool readAccelerometer();

#include <MicroOscUdp.h>
#include "microsmooth.h"
//Function Declaration
WiFiUDP udp;
IPAddress sendIp(192, 168, 0, 255); // <- default not really used, we are using Bonjour (mDNS) to find IP and PORT of touchoscbridge
unsigned int sendPort = 12101; // <- touchosc port

MicroOscUdp<1024> oscUdp(&udp, sendIp, sendPort);
uint8_t midi[4]; 

//// For a PWM to the LEDPIN on the nodeMCU ESP32
#include <analogWrite.h> // From the polyfill analogWrite library for ESP32 //
#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // For the boards that do not have a LED_BUILTIN
#endif

void setup() {
    // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    Serial.begin(115200);
    
    #if defined Display
    displaySetup();
    #endif
    #if defined Accelerometer
    accelerometerSetup();
    #endif
    #if defined PressureSensor
    pressureSensorSetup();
    #endif
    #if defined useButton
    buttonSetup();
    #endif
   
    paramsSetup();
    paramsEEpromSetup();
    //portalInit();
    startPortal();
}

void loop() {
    //wm.process();
    serverListen();
    #if defined Accelerometer
    checkAccelerometer();
    #endif
    #if defined PressureSensor
    readCFSensor(0x6D);   //start conversion and read on pressure sensor at 0x6D address
    #endif
    #if defined useButton
    getButtonState();
    #endif
    #if defined useSensors
    readSensors();
    #endif
    while ((millis() - startMillis) < period);      //waits until period done

}
