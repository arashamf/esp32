#include "main.h"

//static const char *TAG = "main";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/

static const char *TAG = "main";

static uint8_t s_led_state = 0;


void app_main(void)
{
	esp_err_t ret;
	spi_device_handle_t spi;

    // Configure the peripheral according to the LED type
	int led_gpio = CONFIG_BLINK_GPIO;
	gpio_reset_pin(led_gpio); //Сброс инициализации gpio
	gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT); //настройка направления работы данных gpio

	 // Configure SPI bus
	spi_bus_config_t cfg =
	{
			.mosi_io_num = CONFIG_PIN_NUM_MOSI,
			.miso_io_num = -1,
			.sclk_io_num = CONFIG_PIN_NUM_CLK,
			.quadwp_io_num = -1,
			.quadhd_io_num = -1,
			.max_transfer_sz = 0,
			.flags = 0
	 };
	 ret = spi_bus_initialize(HSPI_HOST, &cfg, SPI_DMA_CH_AUTO); //инициализация шины SPI
	 printf ("spi bus initialize: %d\r\n", ret);
	 spi_device_interface_config_t devcfg={
	         .clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz
	         .mode=0,                                //SPI mode 0
	         .spics_io_num=CONFIG_PIN_NUM_CS,               //CS pin
	         .queue_size=7,                          //We want to be able to queue 7 transactions at a time
	         .pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
	   };
	 ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
	 printf ("spi bus add device: %d\r\n", ret);
	 vTaskDelay(1000 / portTICK_PERIOD_MS);
	 init_lcd7735 (spi, 320, 240);
	 lcdFillRGB (spi, BLACK);

    while (1)
    {

    	s_led_state = !s_led_state;
    	gpio_set_level(led_gpio, s_led_state);
    	vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

