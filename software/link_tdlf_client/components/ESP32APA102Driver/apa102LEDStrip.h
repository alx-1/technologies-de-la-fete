#ifndef apa102LEDStrip_H
#define apa102LEDStrip_H

struct apa102LEDStrip
{
	unsigned char *LEDs;
    short int _frameLength;
    short int _endFrameLength;
    short int _numLEDs;
    unsigned char _bytesPerLED;
    short int _counter;
    unsigned char _globalBrightness;
};

#endif