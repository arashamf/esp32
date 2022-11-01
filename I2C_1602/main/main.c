
#include "main.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;
esp_err_t ret;

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

	 vTaskDelay(2000 / portTICK_PERIOD_MS);

    while (1)
    {
    	clear_LCD1602 ();
    	if (s_led_state == 0)
    	{
    		sprintf (lcd_buffer, "Led_on");
    		s_led_state = 1;
    	}
    	else
    	{
    		sprintf (lcd_buffer, "Led_off");
    		s_led_state = 0;
    	}
    	gpio_set_level(led_gpio, s_led_state);

    	xLCDData.x_pos = 5;
    	xLCDData.y_pos = 0;
    	xQueueSendToBack(lcd_string_queue, &xLCDData, 0); //передача в очередь данные для lcd 1602

    	vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

