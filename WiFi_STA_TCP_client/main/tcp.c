#include "tcp.h"
//-------------------------------------------------------------
static const char *TAG = "tcp";
//-----------------------------------------------------------------------------------------------------------------//
typedef struct
{
  unsigned char y_pos;
  unsigned char x_pos;
  char *str;
} qLCDData;

//-----------------------------------------------------------------------------------------------------------------//
xQueueHandle lcd_string_queue = NULL;

//-----------------------------------------------------------------------------------------------------------------//
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

//----------------------------------------------------------------------------------------------------//
void tcp_task(void *pvParameters)
{
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;  //структуры с данными udp-сервера и udp-клиента
	
	TaskHandle_t xLCDTaskHandle = NULL;
	qLCDData xLCDData;
	char lcd_buffer[24];
	xLCDData.x_pos = 2;
	xLCDData.y_pos = 0;
	xLCDData.str = lcd_buffer;
	lcd_string_queue = xQueueCreate(10, sizeof(qLCDData));
	xTaskCreate(vLCDTask, "vLCDTask", 2048, NULL, 2, &xLCDTaskHandle);
	ESP_LOGI(TAG, "Create socket...\n");

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0 ) //протокол tcp
	{
		 ESP_LOGE(TAG, "socket not created\n");
		 vTaskDelete(xLCDTaskHandle);
		 vQueueDelete(lcd_string_queue);
		 vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "Socket created");
	memset(&servaddr, 0, sizeof(servaddr)); //Очистка памяти полей переменных типов структур
	memset(&cliaddr, 0, sizeof(cliaddr));
		
	//Заполнение информации о клиенте
	cliaddr.sin_family    = AF_INET; // IPv4
	cliaddr.sin_addr.s_addr = INADDR_ANY;
	cliaddr.sin_port = htons(CONFIG_CLIENT_PORT);
    
	if (bind(sockfd, (const struct sockaddr *)&cliaddr,  sizeof(struct sockaddr_in)) < 0 ) //Связь сокета с адресом сервера
	{
		ESP_LOGI(TAG,"socket not binded");
	    vTaskDelete(xLCDTaskHandle);
	    vQueueDelete(lcd_string_queue);
	    vTaskDelete(NULL);
	}
	ESP_LOGI(TAG,"socket was binded");
	//Заполнение информации о сервере
	servaddr.sin_family    = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = inet_addr(CONFIG_SERVER_IP);
	servaddr.sin_port = htons(CONFIG_SERVER_PORT);
	
	/*Попытка соединиться с сервером, если соединение прошло удачно, то выводится соответствующее сообщение на дисплей,
	 затем ожидание 2с и попытка передать строку серверу, после чего ожидание ещё 2с*/
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) >= 0)
	{
	    sprintf (lcd_buffer, "Connected");
	    xQueueSendToBack(lcd_string_queue, &xLCDData, 0);
	    vTaskDelay (2000 / portTICK_RATE_MS);
	    sprintf (lcd_buffer, "Hello from ESP!\n");
	    write(sockfd,(void *) lcd_buffer, strlen(lcd_buffer));
		vTaskDelay (2000 / portTICK_RATE_MS);
	}
	shutdown(sockfd, 0);
	close(sockfd);
	sprintf(lcd_buffer, "Disonnected");
	xQueueSendToBack(lcd_string_queue, &xLCDData, 0);
	vTaskDelay( 2000 / portTICK_RATE_MS);
	vTaskDelete(xLCDTaskHandle);
	vQueueDelete(lcd_string_queue);
	vTaskDelete(NULL);
}


