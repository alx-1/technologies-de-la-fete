#include "apa102LEDStrip.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void initLEDs(struct apa102LEDStrip *ledObject, short int numLEDs, unsigned char bytesPerLED, unsigned char globalBrightness)
{  
  ledObject->_numLEDs = numLEDs;
  ledObject->_bytesPerLED = bytesPerLED;
  ledObject->_endFrameLength = 1;//round( (numLEDs/2)/8 );
  ledObject->_frameLength = (1+numLEDs+ledObject->_endFrameLength)*bytesPerLED;
  ledObject->_globalBrightness = globalBrightness;
  ledObject->LEDs = (unsigned char *)malloc(ledObject->_frameLength*sizeof(unsigned char)); 
  
  //Start Frame
  ledObject->LEDs[0] = 0;
  ledObject->LEDs[1] = 0;
  ledObject->LEDs[2] = 0;
  ledObject->LEDs[3] = 0;
  //Driver frame+PIXEL frames
  for(ledObject->_counter=ledObject->_bytesPerLED; ledObject->_counter<ledObject->_frameLength-(ledObject->_endFrameLength*ledObject->_bytesPerLED); ledObject->_counter+=ledObject->_bytesPerLED)
  {
    ledObject->LEDs[ledObject->_counter] = ledObject->_globalBrightness;
    ledObject->LEDs[ledObject->_counter+1] = 0;
    ledObject->LEDs[ledObject->_counter+2] = 0;
    ledObject->LEDs[ledObject->_counter+3] = 0;
  }
  //END frames
  for(ledObject->_counter=ledObject->_frameLength-(ledObject->_endFrameLength*ledObject->_bytesPerLED); ledObject->_counter<ledObject->_frameLength; ledObject->_counter+=ledObject->_bytesPerLED)
  {
    ledObject->LEDs[ledObject->_counter] = 255;
    ledObject->LEDs[ledObject->_counter+1] = 255;
    ledObject->LEDs[ledObject->_counter+2] = 255;
    ledObject->LEDs[ledObject->_counter+3] = 255;
  }
}
void setPixel(struct apa102LEDStrip *ledObject, short int pixelIndex, unsigned char *pixelColour)
{
  ledObject->_counter = 4*(pixelIndex+1);
  ledObject->LEDs[ ledObject->_counter + 1 ] = pixelColour[2];
  ledObject->LEDs[ ledObject->_counter + 2 ] = pixelColour[1];
  ledObject->LEDs[ ledObject->_counter + 3 ] = pixelColour[0];
}
void getPixel(struct apa102LEDStrip *ledObject, short int pixelIndex, unsigned char *pixelColour)
{
  ledObject->_counter = 4*(pixelIndex+1);
  pixelColour[2] = ledObject->LEDs[ ledObject->_counter + 1 ];
  pixelColour[1] = ledObject->LEDs[ ledObject->_counter + 2 ];
  pixelColour[0] = ledObject->LEDs[ ledObject->_counter + 3 ];
}
