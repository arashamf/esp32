
#include "main.h"
#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;
char lcd_buffer[16];
esp_err_t ret;

static const char *TAG = "main";
extern void udp_task(void *pvParameters);

//-----------------------------------------------------------------------------------------------------------------//
xQueueHandle lcd_string_queue = NULL; //очередь передачи данных для lcd

//-----------------------------------------------------------------------------------------------------------------//
void vLCDTask(void* arg)
{
  BaseType_t xStatus;
  struct qLCDData xReceivedData;
  for(;;)
  {
    xStatus = xQueueReceive(lcd_string_queue, &xReceivedData, 10000 /portTICK_RATE_MS);
    if (xStatus == pdPASS) //если данные получены из очереди успешно
    {
      LCD_SetPos(xReceivedData.x_pos,xReceivedData.y_pos);
      LCD_String(xReceivedData.str);
    }
  }
}

//-----------------------------------------------------------------------------------------------------------------//
void init_nvs_flas()
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

//-----------------------------------------------------------------------------------------------------------------//
void app_main(void)
{
	int led_gpio = CONFIG_BLINK_GPIO;  // Configure the peripheral according to the LED type
	gpio_reset_pin(led_gpio); //Сброс инициализации gpio
	gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT); //настройка направления работы данных gpio

	init_nvs_flas();

	struct qLCDData xLCDData; //объявление экземпляра структуры данных для отображения на LCD1602
	lcd_string_queue = xQueueCreate(5, sizeof(xLCDData)); //создание очереди для передачи структуры xLCDData
	TaskHandle_t xLCDTaskHandle = NULL;
	xTaskCreate(vLCDTask, "vLCDTask", 2048, NULL, 2, &xLCDTaskHandle); //создание задачи отображения данных на LCD, xLCDTaskHandle-дескриптор задачи

	ret = i2c_ini(); //инициализация i2c
	LCD_ini(); //инициализация дисплея

	vTaskDelay(100 / portTICK_PERIOD_MS);
	xLCDData.str = lcd_buffer;

	wifi_init_sta(); //инициализация режима станции wifi
	xTaskCreate (udp_task, "udp_task", 4096, NULL, 5, NULL);

	while (true)
	{
		if (s_led_state == 0)
		{
			s_led_state = 1;
		}
		else
		{
			s_led_state = 0;
		}
		gpio_set_level(led_gpio, s_led_state);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}
