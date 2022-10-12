#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

//static const char *TAG = "main";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static uint8_t s_led_state = 0;


void app_main(void)
{
	 int led_gpio = CONFIG_BLINK_GPIO;
	 int button_gpio = CONFIG_BUTTON_GPIO;

//	 gpio_reset_pin(led_gpio); //Сброс инициализации gpio
//	 gpio_reset_pin(button_gpio);

	 gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT); //настройка направления работы данных gpio
	 gpio_set_direction(button_gpio, GPIO_MODE_INPUT);
	 gpio_set_pull_mode(button_gpio, GPIO_PULLUP_ONLY);
    /* Configure the peripheral according to the LED type */

    while (1)
    {
    	if(gpio_get_level(button_gpio))
    		gpio_set_level(led_gpio, 0);
    	else
    		gpio_set_level(led_gpio, 1);
    //	s_led_state = !s_led_state;
    //	gpio_set_level(led_gpio, s_led_state);
    	vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

