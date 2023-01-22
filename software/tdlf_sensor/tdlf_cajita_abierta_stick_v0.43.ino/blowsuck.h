#if defined PressureSensor
#include <Wire.h>

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

/// BlowSuck ///
String storedBlowChan;
String storedBlowCC;
int storedBlowChanInt;
int storedBlowCCInt;

String storedSuckChan;
String storedSuckCC;
int storedSuckChanInt;
int storedSuckCCInt;

bool checkBlow = false;
bool checkSuck = false;

bool readCFSensor(byte sensorAddress);
       
        //  blowValue = sensorData[sensorIndex+6]; // fill in the values from the sensorData array here
        //  suckValue = sensorData[sensorIndex+7];

void pressureSensorSetup(){
    Wire.begin(); // for the pressure sensor
    // filter
    //ptr = ms_init(EMA);
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

          //Serial.print("suckValue : ");
          //Serial.println(suckValue);
 
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
  
  return 0;
}  
      
    

#endif
