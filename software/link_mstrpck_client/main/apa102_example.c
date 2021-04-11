#include <apa102.h>

///// SPI CONFIG TTGO
///// HSPI
///// HSPI MOSI 23
///// HSPI SCK 18


static int docolors()
{
	for (int i = 0; i < LEDSTRIP.count; i++)
	{
		int v = (0xFF * ((i + LEDSTRIP.phase) % LEDSTRIP.count)) / LEDSTRIP.count;
		LEDSTRIP.leds[i] = RGBL(v, v + 0x44, v + 0x88, 0xFF);
	}
	return ESP_OK;
}

void app_main()
{
	LEDSTRIP.init(48);
	LEDSTRIP.refresh = docolors;

	for (;;)
	{
		LEDSTRIP.update();
		vTaskDelay(10);
	}

	// for show
	LEDSTRIP.deinit();
}
