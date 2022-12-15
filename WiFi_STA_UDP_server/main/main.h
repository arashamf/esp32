#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_
//---------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "lcd1602.h"
#include "i2c_user.h"
#include "wifi.h"
#include "udp.h"
#include "nvs_flash.h"
//---------------------------------------------------------------------

#define BLINK_GPIO CONFIG_BLINK_GPIO

#endif /* MAIN_MAIN_H_ */
