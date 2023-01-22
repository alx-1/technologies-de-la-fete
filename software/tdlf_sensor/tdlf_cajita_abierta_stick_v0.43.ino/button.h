#if defined useButton
#include <ezButton.h>

//// Button to start config
bool configureCajita = false; // A button press will start it

#define SHORT_PRESS_TIME 1000 // 1000 milliseconds
#define LONG_PRESS_TIME  1000 // 1000 milliseconds

ezButton button1(0); // create ezButton object that attach to pin GPIO0 (default button on ESP32s, Node)
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
bool isPressing = false;
bool isLongDetected = false;

void buttonSetup(){
  button1.setDebounceTime(50); // set debounce time to 50 milliseconds
}

void getButtonState(){
  button1.loop(); // MUST call the loop() function first
  int btn1State = button1.getState();

  if(button1.isReleased()){
    Serial.println("The button 1 is released");
    configureCajita = true; // Need to set a flag so we start the config routine
  }
}
#endif
