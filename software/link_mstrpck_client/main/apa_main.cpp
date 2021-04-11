
extern "C" {
#include <apa102.h>
#include <stdbool.h> 
}

///// SPI CONFIG TTGO
///// VSPI
///// VSPI MOSI 23
///// VSPI SCK 18

// LED on
// LED off
// Beat on (lum + 33)
extern "C" {
static bool indicLED = false;
}

extern "C" {

static int docolors()
    {
    printf("indicLED : %d\n", indicLED);
            if(indicLED==true){
                LEDSTRIP.leds[1] = RGBL(0x33, 0x99, 0x44, 0xFF);
                indicLED = false;
                //printf("grr");
                }
            else if (indicLED==false)
                {
                LEDSTRIP.leds[1] = RGBL(0x88, 0x00, 0x22, 0x66);
                indicLED = true;
                //printf("wow");
                }
    return ESP_OK; 
    }
}

extern "C" {
void app_main()
{
	LEDSTRIP.init(1);  // nmbr de leds // flashes when '1'
    printf("---------- led start ------------\n");
	LEDSTRIP.refresh = docolors;
    printf("-------- led init done ----------\n");

	for (;;)
	{
		LEDSTRIP.update(); // calls docolors()

        //printf("boucle\n");        
		
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
	}

	// for show
	LEDSTRIP.deinit();
    printf("-------- led deinit ----------\n");

}
}


////////// merde //
/*
extern "C"{


#include <apa102.h>
}

///// SPI CONFIG TTGO
///// HSPI
///// HSPI MOSI 23
///// HSPI SCK 18

extern "C"{
static int docolors()
{
	for (int i = 0; i < LEDSTRIP.count; i++)
	{
		int v = (0xFF * ((i + LEDSTRIP.phase) % LEDSTRIP.count)) / LEDSTRIP.count;
		LEDSTRIP.leds[i] = RGBL(v, v + 0x44, v + 0x88, 0xFF);
	}
	return ESP_OK;
}
}

extern "C"{
void app_main()
{
	LEDSTRIP.init(16);
	LEDSTRIP.refresh = docolors;

	for (;;)
	{
		LEDSTRIP.update();
		vTaskDelay(10);
	}

	// for show
	LEDSTRIP.deinit();
}
}
*/