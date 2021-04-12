#ifndef ESP32APA102Driver_H
#define ESP32APA102Driver_H

#include "apa102LEDStrip.h"
#include "colourObject.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"

#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18

#define totalPixels 16
#define bytesPerPixel 4
#define maxValuePerColour 16 // 64
#define maxSPIFrameInBytes 8000
#define maxSPIFrequency 10000000

//Main functions
int setupSPI();
void renderLEDs();
//apa102LEDStrip functions
void initLEDs(struct apa102LEDStrip *ledObject, short int numLEDs, unsigned char bytesPerLED, unsigned char globalBrightness);
void setPixel(struct apa102LEDStrip *ledObject, short int pixelIndex, unsigned char *pixelColour);
void getPixel(struct apa102LEDStrip *ledObject, short int pixelIndex, unsigned char *pixelColour);
//colourObject functions
void initSimpleColourObject(struct colourObject *thisObject, unsigned char maxValue);
void initComplexColourObject(struct colourObject *thisObject, unsigned char maxValue, unsigned char colourBlockCount, unsigned char *rgbColourArray);
void gradientGenerator(struct colourObject *thisObject, unsigned short int colourIndex, unsigned short int bandwidth);
void getColour(struct colourObject *thisObject, short int colourIndex, unsigned char *colourBlock);

#endif