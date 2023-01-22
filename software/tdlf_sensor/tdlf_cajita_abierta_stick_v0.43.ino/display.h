////////////// DISPLAY //////////
#if defined Display
#include <SPI.h>
//#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif
/////////////////////////////

void displaySetup(){
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
}
