#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_
//---------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <driver/spi_master.h>
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "st7735.h"
//---------------------------------------------------------------------

#define BLINK_GPIO CONFIG_BLINK_GPIO

#endif /* MAIN_MAIN_H_ */
