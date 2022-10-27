#include "main.h"

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

static char LCD_buffer [30];
static uint8_t s_led_state = 0;

esp_err_t ret;
spi_device_handle_t spi;

xQueueHandle lcd_string_queue = NULL;
xSemaphoreHandle xBinSem01;

//--------------------------------------------------------------------------------------------------------------------//
typedef struct
{
  unsigned char y_pos;
  unsigned char x_pos;
  uint16_t global_count;
} qLCDData;

//--------------------------------------------------------------------------------------------------------------------//
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(CONFIG_PIN_NUM_DC, dc);
}

//--------------------------------------------------------------------------------------------------------------------//
void LCD_display_init ()
{
	init_lcd7735 (spi);
	LCD_SetRotation(spi, 2); //горизонтальная ориентация
	LCD_FillScreen(spi, BLUE); //цвет фона
	LCD_SetTextColor(WHITE); //цвет текста
	LCD_SetBackColor(RED);
	LCD_SetFont(&Font12); //шрифт
}

//---------------------------------------функция-обработчик прерываний от таймера---------------------------------------//
static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
	BaseType_t high_task_awoken = pdFALSE;
	xSemaphoreGiveFromISR(xBinSem01, &high_task_awoken); //выдача семафора
	return high_task_awoken == pdTRUE;
}

//--------------------------------------------------------------------------------------------------------------------//
static void hw_timer_init(int group, int timer, bool auto_reload, int timer_interval_sec)
{
	timer_config_t config =
	{
	      .divider = TIMER_DIVIDER,
	      .counter_dir = TIMER_COUNT_UP,
	      .counter_en = TIMER_PAUSE,
	      .alarm_en = TIMER_ALARM_EN,
	      .auto_reload = auto_reload,
	};
	timer_init(group, timer, &config);
	timer_set_counter_value(group, timer, 0); //сброс начального значения счётчика в ноль
	timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE); //установка интервала срабатывания таймера
	timer_enable_intr(group, timer); //включение прерываний от таймера
	timer_isr_callback_add(group, timer, timer_group_isr_callback, NULL, 0); //регистрация функции-обработчика прерывания для таймера
	timer_start(group, timer); //запуск таймера
}

//--------------------------------------------------------------------------------------------------------------------//
void vLCDTask(void* arg)
{
  BaseType_t xStatus;
  qLCDData xReceivedData;
  LCD_FillScreen(spi, BLUE); //цвет фона

  for(;;)
  {
	xStatus = xQueueReceive(lcd_string_queue, &xReceivedData, 10000 /portTICK_RATE_MS); //получение данных из очереди
    if (xStatus == pdPASS)
    {
    	sprintf (LCD_buffer, "count=%u", xReceivedData.global_count);
    	LCD_ShowString (spi, 5, 10, LCD_buffer);
    }
  }
}


//--------------------------------------------------------------------------------------------------------------------//
static void count_task (void* arg)
{
	uint16_t count = 0;
	qLCDData xLCDData; //объявление экземпляра структуры

	for(;;)
	{
	    xSemaphoreTake(xBinSem01, portMAX_DELAY); //получение семафора от таймера раз в 1 с
	    xLCDData.x_pos = 0;
	    xLCDData.y_pos = 0;
	    xLCDData.global_count = count;
	    xQueueSendToBack(lcd_string_queue, &xLCDData, 0); //передача данных в очередь для таска vLCDTask
	    count++;
	    if (count > 99)
	    {
	    	count = 0;
	    }

	 }
}

//--------------------------------------------------------------------------------------------------------------------//
void app_main(void)
{

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
	 vTaskDelay(50 / portTICK_PERIOD_MS);

	 LCD_display_init ();
	 sprintf (LCD_buffer, "program start");
	LCD_ShowString (spi, 15, 20, LCD_buffer);


	 hw_timer_init(TIMER_GROUP_0, TIMER_0, true, 1); //инициализация таймера с авторелоадом и периодом в 1 с
	 vSemaphoreCreateBinary(xBinSem01); //объявление бинарного семафора
	 if (xBinSem01 != NULL)
		 xTaskCreate(count_task, "count_task", 2048, NULL, 5, NULL);

	 lcd_string_queue = xQueueCreate(5, sizeof (qLCDData)); //создание очереди между таймером и таском vLCDTask
	 xTaskCreate(vLCDTask, "vLCDTask", 1024, NULL, 2, NULL); //создание таска vLCDTask вывода показателей счётчика на дисплей
	 xTaskCreate(count_task, "count_task", 1024, NULL, 2, NULL); //создание таска count_task с счётчиком раз в 1 с

	 vTaskDelay(500 / portTICK_PERIOD_MS);

    while (1)
    {

    	if (s_led_state == 0)
    		s_led_state = 1;
    	else
    		s_led_state = 0;
    	gpio_set_level(led_gpio, s_led_state);
    	vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

