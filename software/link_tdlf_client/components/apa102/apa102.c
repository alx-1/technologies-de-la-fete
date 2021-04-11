#include <apa102.h>

#define TAG "APA102"

inline int apa102_max_transfer(int nleds)
{
	return (8 * ((2 + nleds) * sizeof(apa102_color_t)));
}

static int apa102_init(int n)
{
	APA102.count = (n <= 0) ? CONFIG_LMTZ_APA102_NUM_LEDS : n;
	APA102.transaction.length = apa102_max_transfer(APA102.count);
	APA102.bus_config.max_transfer_sz = APA102.transaction.length;
	size_t size = (APA102.count + 2) * sizeof(apa102_color_t);
	APA102.txbuffer = heap_caps_malloc(size, MALLOC_CAP_DMA | MALLOC_CAP_32BIT);
	memset(APA102.txbuffer, 0, size);
	
	for (int i=0; i<APA102.count; i++) APA102.txbuffer[1+i] = 0xE0000000;
	spi_bus_initialize(APA102.spi_host, &APA102.bus_config, APA102.dma_channel);
	spi_bus_add_device(APA102.spi_host, &APA102.dev_config, &APA102.device);

	return ESP_OK;
}

static int apa102_update() 
{
	if (!APA102.transaction.tx_buffer)
	{
		APA102.transaction.tx_buffer = APA102.txbuffer;
	}

	spi_device_queue_trans(APA102.device, &APA102.transaction, portMAX_DELAY);
	if (APA102.refresh) APA102.refresh();
	spi_transaction_t* t;
	spi_device_get_trans_result(APA102.device, &t, portMAX_DELAY);
	// APA102.phase += 1;
	 // printf("-------- update ?!? ------\n");

	return ESP_OK;
}

static int apa102_deinit()
{
	free(APA102.txbuffer);
	return ESP_OK;
}

apa102_driver_t APA102 = { 
	.phase = 0, 
	.device = 0, 
	.spi_host = CONFIG_LMTZ_APA102_SPI_HOST, 
	.dma_channel = CONFIG_LMTZ_APA102_DMA_CHANNEL, 
	.bus_config = { 
		.miso_io_num = -1, 
		.mosi_io_num = CONFIG_LMTZ_APA102_PIN_MOSI, 
		.sclk_io_num = CONFIG_LMTZ_APA102_PIN_SCLK, 
		.quadwp_io_num = -1, 
		.quadhd_io_num = -1, 
	}, 
	.dev_config = { 
		.clock_speed_hz = CONFIG_LMTZ_APA102_CLOCK_SPEED, 
		.mode = 3, 
		.spics_io_num = -1,
		.queue_size = 1, 
	},

	.init = apa102_init,
	.update = apa102_update,
	.refresh = NULL,
	.deinit = apa102_deinit,
};

