#include "main.h"

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds

static char LCD_buffer [30];
static uint8_t s_led_state = 0;

esp_err_t ret;
spi_device_handle_t spi;

static xQueueHandle s_timer_queue; //очередь таймера
//---------------------------------------глобальная структура для свойств таймера---------------------------------------//
typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} timer_info_t;

//--------------------------------------------------------------------------------------------------------------------//
typedef struct {
    timer_info_t info;
    uint64_t timer_counter_value; //значения счётчика таймера
    double time;
} timer_event_t;

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
	LCD_SetTextColor(LIGHTBLUE); //цвет текста
	LCD_SetBackColor(BLUE);
	LCD_SetFont(&Font12); //шрифт
}

//--------------------------------------------------------------------------------------------------------------------//
void vLCDTask(void* arg)
{
  BaseType_t xStatus;
  uint64_t task_counter_value;
  timer_event_t evt; //объявление переменной типа структуры свойств таймера
  uint8_t count = 0;

  for(;;)
  {
	xStatus = xQueueReceive(s_timer_queue, &evt, 1000 /portTICK_RATE_MS); //получение данных из очереди
    if (xStatus == pdPASS)
    {
    	count++;
    	timer_get_counter_value(evt.info.timer_group, evt.info.timer_idx, &task_counter_value);
    	sprintf (LCD_buffer, "counter=%llu, count=%d",task_counter_value, count);

    	printf ("Time   : %8.f s\r\n", evt.time);
    	LCD_FillScreen(spi, BLUE); //цвет фона
    	LCD_ShowString (spi, 5, 10, LCD_buffer);
    	if (count >= 255)
    		count = 0;
    }
  }
}

//---------------------------------------функция-обработчик прерываний от таймера---------------------------------------//
static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
	BaseType_t high_task_awoken = pdFALSE;
	double time = 0;

	timer_info_t *info = (timer_info_t *) args; //указатель на переменную структуры свойств таймера из параметров
	uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(info->timer_group, info->timer_idx); //сохранение значения счётчика
	timer_get_counter_time_sec (info->timer_group, info->timer_idx, &time);
	timer_event_t evt =
	{
	      .info.timer_group = info->timer_group,
	      .info.timer_idx = info->timer_idx,
	      .info.auto_reload = info->auto_reload,
	      .info.alarm_interval = info->alarm_interval,
	      .timer_counter_value = timer_counter_value,
		  .time = time
	  };

	if (!info->auto_reload) //если таймер без перезагрузки, то к значению, взятому из регистра счётчика, добавится интервал и полученное значение занесётся в регистр срабатывания
	{
		timer_counter_value += info->alarm_interval * TIMER_SCALE;
		timer_group_set_alarm_value_in_isr(info->timer_group, info->timer_idx, timer_counter_value);
	}


	xQueueSendFromISR(s_timer_queue, &evt, &high_task_awoken); //отправка указателя на переменную структуры в очередь
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
	timer_info_t *timer_info = calloc(1, sizeof(timer_info_t)); //резервирование памяти
	timer_info->timer_group = group;
	timer_info->timer_idx = timer;
	timer_info->auto_reload = auto_reload;
	timer_info->alarm_interval = timer_interval_sec;
	timer_isr_callback_add(group, timer, timer_group_isr_callback, timer_info, 0); //регистрация функции-обработчика прерывания для таймера
	timer_start(group, timer); //запуск таймера
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
	 LCD_ShowString (spi, 5, 10, LCD_buffer);

	 hw_timer_init(TIMER_GROUP_0, TIMER_0, true, 1); //инициализация таймера с авторелоадом и периодом в 2 с
	 s_timer_queue = xQueueCreate(10, sizeof(timer_event_t)); //создание очереди между таймером и таском vLCDTask
	 xTaskCreate(vLCDTask, "vLCDTask", 2048, NULL, 2, NULL); //создание таска vLCDTask

	 vTaskDelay(1000 / portTICK_PERIOD_MS);

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

