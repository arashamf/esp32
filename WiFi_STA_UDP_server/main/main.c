
#include "main.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

//static uint8_t s_led_state = 0;
char lcd_buffer[16];
esp_err_t ret;

static const char *TAG = "main";
extern void udp_task(void *pvParameters);

//-----------------------------------------------------------------------------------------------------------------//
void init_nvs_flash()
{
	esp_err_t ret = nvs_flash_init(); //Initialize NVS (NVS — это такой раздел памяти контроллера, в котором хранятся некоторые настройки или свойства, так как это такая файловая система, основанная на конструкциях ключ-значение)
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ret = nvs_flash_erase();
		ESP_LOGI(TAG, "nvs_flash_erase: 0x%04x", ret);
		ret = nvs_flash_init();
		ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
	}
	ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
}

//------------------------------------------------
void app_main(void)
{
	int led_gpio = CONFIG_BLINK_GPIO;  // Configure the peripheral according to the LED type
	gpio_reset_pin(led_gpio); //Сброс инициализации gpio
	gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT); //настройка направления работы данных gpio

	init_nvs_flash();

	ret = i2c_ini();
	LCD_ini();
	vTaskDelay(100 / portTICK_PERIOD_MS);

	ret = esp_netif_init();
	ESP_LOGI(TAG, "esp_netif_init: %d", ret);
	ret = esp_event_loop_create_default();
	ESP_LOGI(TAG, "esp_event_loop_create_default: %d", ret);
	wifi_init_sta(); //инициализация режима станции wifi
	xTaskCreate (udp_task, "udp_task", 4096, NULL, 5, NULL);

	vTaskDelay(500 / portTICK_PERIOD_MS);

	while (true)
	{

    	vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}
