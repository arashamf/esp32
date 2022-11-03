
#include "main.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;
esp_err_t ret;

static const char *TAG = "main";

//------------------------------------------------
typedef struct
{
  unsigned char y_pos;
  unsigned char x_pos;
  char *str;
} qLCDData;

//------------------------------------------------
xQueueHandle lcd_string_queue = NULL;

//------------------------------------------------
void vLCDTask(void* arg)
{
  BaseType_t xStatus;
  qLCDData xReceivedData;
  for(;;)
  {
    xStatus = xQueueReceive(lcd_string_queue, &xReceivedData, 10000 /portTICK_RATE_MS);
    if (xStatus == pdPASS)
    {
      LCD_SetPos(xReceivedData.x_pos,xReceivedData.y_pos);
      LCD_String(xReceivedData.str);
    }
  }
}

//------------------------------------------------
void app_main(void)
{
	int led_gpio = CONFIG_BLINK_GPIO;  // Configure the peripheral according to the LED type
	gpio_reset_pin(led_gpio); //Сброс инициализации gpio
	gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT); //настройка направления работы данных gpio

	char lcd_buffer[16];
	qLCDData xLCDData; //объявление экземпляра структуры данных для отображения на LCD1602

	lcd_string_queue = xQueueCreate(10, sizeof(qLCDData));
	xTaskCreate(vLCDTask, "vLCDTask", 2048, NULL, 2, NULL);
	ret = i2c_ini();
	LCD_ini();
	vTaskDelay(100 / portTICK_PERIOD_MS);

	xLCDData.str = lcd_buffer;
	for(uint8_t i=0;i<2;i++)
	{
		xLCDData.x_pos = 0;
	    xLCDData.y_pos = i;
	    sprintf(lcd_buffer,"String %d",i+1);
	    xQueueSendToBack(lcd_string_queue, &xLCDData, 0); //передача в очередь данные для lcd 1602
	 }

	wifi_ap_record_t info;	//переменнаяю типа структуры для получения свойств соединения WiFi

	esp_err_t ret = nvs_flash_init(); //Initialize NVS (NVS — это такой раздел памяти контроллера, в котором хранятся некоторые настройки или свойства, так как это такая файловая система, основанная на конструкциях ключ-значение)
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ret = nvs_flash_erase();
		ESP_LOGI(TAG, "nvs_flash_erase: 0x%04x", ret);
		ret = nvs_flash_init();
		ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
	}
	ESP_LOGI(TAG, "nvs_flash_init: 0x%04x", ret);
	wifi_init_sta();

	vTaskDelay(1000 / portTICK_PERIOD_MS);

	while (true)
	{
//    	clear_LCD1602 ();
    	ESP_LOGI(TAG, "Heap free size:%d", xPortGetFreeHeapSize());
    	ret = esp_wifi_sta_get_ap_info(&info); //отслеживание состояние соединения WiFi
    	ESP_LOGI(TAG, "esp_wifi_sta_get_ap_info: 0x%04x", ret);

    	if(ret==0)
    	{
    		ESP_LOGI(TAG, "SSID:%s", info.ssid);
    		gpio_set_level(CONFIG_BLINK_GPIO, 0);
    	}
    	vTaskDelay(5000 / portTICK_PERIOD_MS);
    	esp_wifi_disconnect();
	}
}
