#include "main.h"

static char LCD_buffer [30];
static uint8_t s_led_state = 0;

esp_err_t ret;
spi_device_handle_t spi;

xQueueHandle lcd_string_queue = NULL;
static EventGroupHandle_t timer_event_group;

#define TIMER1_EVT BIT0 //макросы для битов событий
#define TIMER2_EVT BIT1
#define TIMER3_EVT BIT2

//--------------------------------------------------------------------------------------------------------------------//
typedef struct
{
  unsigned char y_pos;
  unsigned char x_pos;
  char * ptr;
} qLCDData;


//--------------------------------------------------------------------------------------------------------------------//
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(CONFIG_PIN_NUM_DC, dc);
}

//--------------------------------------------------------------------------------------------------------------------//
void spi_init (spi_device_handle_t spi)
{
	esp_err_t ret;
	spi_bus_config_t cfg = 				// Configure SPI bus
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
	 spi_device_interface_config_t devcfg = 				//настройка устройства SPI
	 {
			 .clock_speed_hz=10*1000*1000,         		//Clock out at 10 MHz
	         .mode=0,                              		 //SPI mode 0
	         .spics_io_num=CONFIG_PIN_NUM_CS,			//CS pin
	         .queue_size=7,                        		//We want to be able to queue 7 transactions at a time
	         .pre_cb = lcd_spi_pre_transfer_callback,  	//Specify pre-transfer callback to handle D/C line
	 };
	 ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
	 printf ("spi bus add device: %d\r\n", ret);
	 vTaskDelay(50 / portTICK_PERIOD_MS);
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

//--------------------------------------------------------------------------------------------------------------------//
void vLCDTask(void* arg)
{
  BaseType_t xStatus;
  qLCDData xReceivedData;

  for(;;)
  {
	xStatus = xQueueReceive(lcd_string_queue, &xReceivedData, 10000 /portTICK_RATE_MS); //получение данных из очереди
    if (xStatus == pdPASS)
    {

    	LCD_ShowString (spi, xReceivedData.x_pos, xReceivedData.y_pos, xReceivedData.ptr);
    }
  }
}

//--------------------------------------------------------------------------------------------------------------------//
static void periodic_timer1_callback(void* arg)
{
//	printf ("timer1 event\r\n");
	xEventGroupSetBits(timer_event_group, TIMER1_EVT);
}

//--------------------------------------------------------------------------------------------------------------------//
static void periodic_timer2_callback(void* arg)
{
	xEventGroupSetBits(timer_event_group, TIMER2_EVT);
}

//--------------------------------------------------------------------------------------------------------------------//
static void periodic_timer3_callback(void* arg)
{
	xEventGroupSetBits(timer_event_group, TIMER3_EVT);
}


//--------------------------------------------------------------------------------------------------------------------//
 void count_task (void* arg)
{
	uint16_t count = 0;
	char buffer [15] = {0};
	qLCDData xLCDData; //объявление экземпляра структуры
	xLCDData.x_pos = 15;
	xLCDData.y_pos = 20;
	xLCDData.ptr = buffer;
	LCD_FillScreen(spi, BLUE);

	for(;;)
	{
		EventBits_t bits = xEventGroupWaitBits (timer_event_group,
		TIMER1_EVT | TIMER3_EVT,// | TIMER3_EVT, //2 оператор указывает биты для проверки внутри группы событий. Например, чтобы дождаться бита 0 и бита 2, установите значение 2 оператора равным 0x05.
		pdTRUE, //если pdTRUE, то любые биты во втором операторе, установленные в группе событий, будут очищены до возврата xEventGroupWaitBits(), если условие ожидания было выполнено. Если параметр=false, то биты, установленные в группе событий, не изменяются при возврате вызова xEventGroupWaitBits()
		pdFALSE, //если pdTRUE, тогда xEventGroupWaitBits() вернет бит, когда будут установлены все биты во 2 операторе, либо истечет указанное время блока. Если установлено false, то функция xEventGroupWaitBits() вернет бит, когда будет установлен любой из битов, установленных в uxBitsToWaitFor, или истечет указанное время блока.
		portMAX_DELAY); //время блокировки
		xLCDData.x_pos = 15;
	    if (bits & TIMER1_EVT)
	    {
	    	sprintf (buffer, "TIMER1 = %2d", count);
	    	xLCDData.y_pos = 20;
	    }
	    if (bits & TIMER2_EVT)
	    {
	    	sprintf (buffer, "TIMER2 = %2d", count);
	    	xLCDData.y_pos = 35;
	    }
	    if (bits & TIMER3_EVT)
	    {
	    	sprintf (buffer, "TIMER3 = %2d", count);
	    	xLCDData.y_pos = 50;
	    }
	    xQueueSendToBack(lcd_string_queue, &xLCDData, 0);
//	    vTaskDelay(1000 / portTICK_PERIOD_MS);
	    count++;
	}
}


//--------------------------------------------------------------------------------------------------------------------//
void app_main(void)
{

	int led_gpio = CONFIG_BLINK_GPIO; // Configure the peripheral according to the LED type
	gpio_reset_pin(led_gpio); //Сброс инициализации gpio
	gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT); //настройка направления работы данных gpio

//	spi_init (spi);
	esp_err_t ret;
	spi_bus_config_t cfg = 				// Configure SPI bus
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
	 spi_device_interface_config_t devcfg = 				//настройка устройства SPI
	 {
			 .clock_speed_hz=10*1000*1000,         		//Clock out at 10 MHz
	         .mode=0,                              		 //SPI mode 0
	         .spics_io_num=CONFIG_PIN_NUM_CS,			//CS pin
	         .queue_size=7,                        		//We want to be able to queue 7 transactions at a time
	         .pre_cb = lcd_spi_pre_transfer_callback,  	//Specify pre-transfer callback to handle D/C line
	 };
	 ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
	 printf ("spi bus add device: %d\r\n", ret);

	LCD_display_init ();
	sprintf (LCD_buffer, "program start");
	LCD_ShowString (spi, 15, 20, LCD_buffer);

	lcd_string_queue = xQueueCreate(5, sizeof (qLCDData)); //создание очереди между таймером и таском vLCDTask
	printf ("lcd_string_queue create\r\n");
	vTaskDelay(500 / portTICK_PERIOD_MS);

	xTaskCreate(vLCDTask, "vLCDTask", 1024, NULL, 2, NULL); //создание таска vLCDTask
	printf ("vLCDTask create\r\n");
	vTaskDelay(500 / portTICK_PERIOD_MS);

	timer_event_group = xEventGroupCreate();
	const esp_timer_create_args_t periodic_timer1_args =  //переменная типа структуры параметров таймера1 с указателем на функцию обратного вызова
	{
		 .callback = &periodic_timer1_callback,
		.name = "periodic1"
	};
	const esp_timer_create_args_t periodic_timer2_args =
	{
		.callback = &periodic_timer2_callback,
		.name = "periodic2"
	};
	const esp_timer_create_args_t periodic_timer3_args =
	{
		.callback = &periodic_timer3_callback,
		.name = "periodic3"
	};
	esp_timer_handle_t periodic_timer1, periodic_timer2, periodic_timer3;

	ret = esp_timer_create(&periodic_timer1_args, &periodic_timer1);
	ret = esp_timer_start_periodic(periodic_timer1, 3000000);
	printf ("timer1 start\r\n");
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	ret = esp_timer_create(&periodic_timer2_args, &periodic_timer2);
	ret = esp_timer_start_periodic(periodic_timer2, 3000000);
	printf ("timer2 start\r\n");
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	ret = esp_timer_create(&periodic_timer3_args, &periodic_timer3);
	ret = esp_timer_start_periodic(periodic_timer3, 3000000);
	printf ("timer3 start\r\n");
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	xTaskCreate(count_task, "count_task", 2048, NULL, 5, NULL); //создание таска count_task
	printf ("count_task create\r\n");
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

