#if defined useAnalogSensors

int sensor1Value = 0; 
int sensor2Value = 0; 
int sensor3Value = 0; 
int sensor4Value = 0; 

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

bool checkSensor1 = false;
bool checkSensor2 = false;
bool checkSensor3 = false;
bool checkSensor4 = false;

void readSensors(){
  //if ( checkSensor1 == true | checkSensor2 == true | checkSensor3 == true | checkSensor4 == true ){
          // if (playback == 0 ) {
          // check the sensors here
          sensor1Value = analogRead(32);
          //Serial.print("sensor1Value : ");
          //Serial.println(sensor1Value);
          sensor2Value = analogRead(33);
          //Serial.print("sensor2Value : ");
          //Serial.println(sensor2Value);
          sensor3Value = analogRead(34);
          //Serial.print("sensor3Value : ");
          //Serial.println(sensor3Value);
          sensor4Value = analogRead(35);
          //Serial.print("sensor4Value : ");
          //Serial.println(sensor4Value);
          
//        } else {
//          sensor1Value = sensorData[sensorIndex+8]; // Read the values from the array
//          sensor2Value = sensorData[sensorIndex+9];
//          sensor3Value = sensorData[sensorIndex+10];
//          sensor4Value = sensorData[sensorIndex+11];
//        }   
 //    }
}


#endif
