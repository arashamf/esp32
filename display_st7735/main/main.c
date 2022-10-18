#include "main.h"

static const char *TAG = "main";
char LCD_buffer [15];

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/

void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(CONFIG_PIN_NUM_DC, dc);
}

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
			.max_transfer_sz = 16*320*2+8,
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
	 init_lcd7735 (spi);
	 LCD_SetRotation(spi, 2); //горизонтальная ориентация
	 LCD_FillScreen(spi, BLACK);
	 vTaskDelay(1000 / portTICK_PERIOD_MS);

    while (1)
    {

    	if (s_led_state == 0)
    		s_led_state = 1;
    	else
    		s_led_state = 0;
    	gpio_set_level(led_gpio, s_led_state);

    	LCD_SetTextColor(LIGHTBLUE);
    	LCD_SetBackColor(BLUE);
    	sprintf (LCD_buffer, "esp32");

    	LCD_FillScreen(spi, BLUE);
    	LCD_SetFont(&Font24);
    	LCD_ShowString (spi, 5, 10, LCD_buffer);
    	vTaskDelay(500 / portTICK_PERIOD_MS);

    	LCD_FillScreen(spi, BLUE);
    	LCD_SetFont(&Font20);
    	LCD_ShowString (spi, 5, 10, LCD_buffer);
    	vTaskDelay(500 / portTICK_PERIOD_MS);

    	LCD_FillScreen(spi, BLUE);
    	LCD_SetFont(&Font16);
    	LCD_ShowString (spi, 5, 10, LCD_buffer);
    	vTaskDelay(500 / portTICK_PERIOD_MS);

    	LCD_FillScreen(spi, BLUE);
    	LCD_SetFont(&Font12);
    	LCD_ShowString (spi, 5, 10, LCD_buffer);
    	vTaskDelay(500 / portTICK_PERIOD_MS);

    	LCD_FillScreen(spi, BLUE);
    	LCD_SetFont(&Font8);
    	LCD_ShowString (spi, 5, 10, LCD_buffer);
    	vTaskDelay(500 / portTICK_PERIOD_MS);


    }
}

