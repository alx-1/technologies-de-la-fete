#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
WiFiManager wm;
#include <EEPROM.h>

/// Save params
#include "FS.h"
#include "LITTLEFS.h" 
#define FORMAT_LITTLEFS_IF_FAILED true
File fileToAppend;
// LITTLEFS.format(); // Only do this once // doesn't work
String esid;
String epass = "";

WiFiManagerParameter sensor1("sensor1", "sensor 1 ", "", 30);
WiFiManagerParameter sensor2("sensor2", "sensor 2 ", "", 30);

void paramsEEpromSetup(){
  EEPROM.begin(512); //Initialasing EEPROM 
  for (int i = 0; i < 32; ++i){
    esid += char(EEPROM.read(i));
  }
  for (int i = 32; i < 64; ++i){
    epass += char(EEPROM.read(i));
  }
  for (int i = 64; i < 66; ++i){
    storedAccelxChan += char(EEPROM.read(i));
  }
  for (int i = 66; i < 69; ++i){
    storedAccelxCC += char(EEPROM.read(i));
  }

 // y
  for (int i = 69; i < 71; ++i){
    storedAccelyChan += char(EEPROM.read(i));
  }
  for (int i = 71; i < 74; ++i){
    storedAccelyCC += char(EEPROM.read(i));
  }
  
  // z
  for (int i = 74; i < 76; ++i){
    storedAccelzChan += char(EEPROM.read(i));
  }
  for (int i = 76; i < 79; ++i){
    storedAccelzCC += char(EEPROM.read(i));
  }

  // Pan
  for (int i = 79; i < 81; ++i){
    storedPanChan += char(EEPROM.read(i));
  }
  for (int i = 81; i < 84; ++i){
    storedPanCC += char(EEPROM.read(i));
  }

  // Tilt
  for (int i = 84; i < 86; ++i){
    storedTiltChan += char(EEPROM.read(i));
  }
  for (int i = 86; i < 89; ++i){
    storedTiltCC += char(EEPROM.read(i));
  }

  // Roll
  for (int i = 89; i < 91; ++i){
    storedRollChan += char(EEPROM.read(i));
  }
  for (int i = 91; i < 94; ++i){
    storedRollCC += char(EEPROM.read(i));
  }

  // Blow
  for (int i = 256; i < 258; ++i){
    storedBlowChan += char(EEPROM.read(i));
  }
  for (int i = 258; i < 261; ++i){
    storedBlowCC += char(EEPROM.read(i));
  }

  // Suck
  for (int i = 261; i < 263; ++i){
    storedSuckChan += char(EEPROM.read(i));
  }
  for (int i = 263; i < 266; ++i){
    storedSuckCC += char(EEPROM.read(i));
  }

  // Sensors 
  for (int i = 266; i < 268; ++i){
    storedSensor1Chan += char(EEPROM.read(i));
  }
  for (int i = 268; i < 271; ++i){
    storedSensor1CC += char(EEPROM.read(i));
  }

  for (int i = 271; i < 273; ++i){
    storedSensor2Chan += char(EEPROM.read(i));
  }
  for (int i = 273; i < 276; ++i){
    storedSensor2CC += char(EEPROM.read(i));
  }

  for (int i = 276; i < 278; ++i){
    storedSensor3Chan += char(EEPROM.read(i));
  }
  for (int i = 278; i < 281; ++i){
    storedSensor3CC += char(EEPROM.read(i));
  }

  for (int i = 281; i < 283; ++i){
    storedSensor4Chan += char(EEPROM.read(i));
  }
  for (int i = 283; i < 286; ++i){
    storedSensor4CC += char(EEPROM.read(i));
  }
 
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



void saveParamsCallback () {
  Serial.println("Get sensor 1:");
  Serial.print(sensor1.getID());
  Serial.print(" : ");
  Serial.println(sensor1.getValue());
  Serial.println("Get sensor 2:");
  Serial.print(sensor2.getID());
  Serial.print(" : ");
  Serial.println(sensor2.getValue());
}

void paramsSetup() {
  //wm.resetSettings(); //reset settings - wipe credentials for testing
    wm.addParameter(&sensor1);
    wm.addParameter(&sensor2);

    wm.setConfigPortalBlocking(false);
    wm.setSaveParamsCallback(saveParamsCallback);

    wm.setMinimumSignalQuality(30);
    // wm.setCustomHeadElement("<style>html{filter: invert(100%); -webkit-filter: invert(100%);}</style>");
    //wm.setCustomHeadElement("<style>html {background-color: black;}</style>");
}
