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

    // Configure the peripheral according to the LED type
	int led_gpio = CONFIG_BLINK_GPIO;
	int button_gpio = CONFIG_BUTTON_GPIO;

	gpio_reset_pin(led_gpio); //Сброс инициализации gpio
	gpio_reset_pin(button_gpio);

	gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT); //настройка направления работы данных gpio
	gpio_set_direction(button_gpio, GPIO_MODE_INPUT);
	gpio_set_pull_mode(button_gpio, GPIO_PULLUP_ONLY);

	 // Configure SPI bus
	spi_bus_config_t cfg =
	{
			.mosi_io_num = CONFIG_PIN_NUM_MOSI,
			.miso_io_num = CONFIG_PIN_NUM_MISO,
			.sclk_io_num = CONFIG_PIN_NUM_CLK,
			.quadwp_io_num = -1,
			.quadhd_io_num = -1,
			.max_transfer_sz = 0,
			.flags = 0
	 };
	 ret = spi_bus_initialize(HSPI_HOST, &cfg, SPI_DMA_CH_AUTO); //инициализация шины SPI
	 printf ("spi bus initialize: %d\r\n", ret);
	 w25q_t dev; //объявление структуры подключения устройства
	 w25_ini (&dev, HSPI_HOST, CONFIG_PIN_NUM_CS); //передачи ф-ии инициализации подключенного устройства
	 W25_Read_ID(&dev);
	 W25_Read_SR(&dev);
	 vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1)
    {

    	s_led_state = !s_led_state;
    	gpio_set_level(led_gpio, s_led_state);
    	vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

