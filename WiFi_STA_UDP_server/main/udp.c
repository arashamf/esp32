#include "udp.h"
//-------------------------------------------------------------
static const char *TAG = "udp";
//-----------------------------------------------------------------------------------------------------------------//
typedef struct
{
  unsigned char y_pos;
  unsigned char x_pos;
  char clear_flag;
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

    	if (xReceivedData.clear_flag == 1)
    	{
    		 clear_LCD1602 ();
    		 xReceivedData.clear_flag = 0;
    	}
    	LCD_SetPos(xReceivedData.x_pos,xReceivedData.y_pos);
    	LCD_String(xReceivedData.str);
    }
  }
}

//----------------------------------------------------------------------------------------------------//
void udp_task(void *pvParameters)
{
	//TaskHandle_t xRecvTask = NULL; //дескрипторы для задач приёма пакетов
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;  //структуры с данными udp-сервера и udp-клиента
	
	TaskHandle_t xLCDTaskHandle = NULL;
	char buffer[10] = {};
	qLCDData xLCDData;
	char lcd_buffer[16];
	xLCDData.x_pos = 2;
	xLCDData.y_pos = 0;
	xLCDData.clear_flag = 0;
	xLCDData.str = lcd_buffer;
	lcd_string_queue = xQueueCreate(10, sizeof(qLCDData));
	xTaskCreate(vLCDTask, "vLCDTask", 2048, NULL, 2, &xLCDTaskHandle);
	//бесконечный цикл, в котором попытаемся создать сокет, если данная операция пройдёт неудачно, то выведем в терминале соответствующее сообщение и выйдем из цикла, 
	//а при успешном создании выведем другое сообщение
	while(1)
	{
		ESP_LOGI(TAG, "Create socket...\n");
		if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0 ) 
		{
			ESP_LOGE(TAG, "socket not created\n");
			break;
		}
		ESP_LOGI(TAG, "Socket created");
		memset(&servaddr, 0, sizeof(servaddr)); //Очистка памяти полей переменных типов структур
		memset(&cliaddr, 0, sizeof(cliaddr));
		
		uint32_t client_addr_len = sizeof(cliaddr);
    
		servaddr.sin_family    = AF_INET; // Заполнение информации о сервере 
		servaddr.sin_addr.s_addr = INADDR_ANY;
		servaddr.sin_port = htons(CONFIG_SERVER_PORT);
    
		if (bind(sockfd, (const struct sockaddr *)&servaddr,  sizeof(struct sockaddr_in)) < 0 ) //Связь сокета с адресом сервера
		{
			ESP_LOGI(TAG,"socket not binded");
			shutdown(sockfd, 0);
			close(sockfd);
			break;
		}
		ESP_LOGI(TAG,"socket binded");
	
		/*цикл, с попыткой приёма пакета от клиента, получение информации о клиенте и запись её в переменную соответствующей структуры, 
		формирование строки с числом, являющейся разностью числа 32767 и пришедшего от клиента числа, отправка данного числа клиенту в виде пакета UDP, 
		а также отображение принятого числа на дисплее при помощи очереди*/
		while(1)
		{
			recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cliaddr, &client_addr_len);
			snprintf (lcd_buffer, sizeof(lcd_buffer), "rcv:%d", *(short*)buffer);
			if (((*(short*)buffer%1000) == 0) || ((*(short*)buffer) == 1))
				xLCDData.clear_flag = 1;
			else
				xLCDData.clear_flag = 0;
			*(short*)buffer = 32767 - *(short*)buffer;
			sendto(sockfd, buffer, 2,  0, (struct sockaddr*) &cliaddr,  sizeof(cliaddr));
			xQueueSendToBack(lcd_string_queue, &xLCDData, 0);
		}
		/*Выход из бесконечного цикла (хотя реально, скорее всего, из него никогда не выйдем) и, если сокет существует, то попробуем его закрыть и, 
		так как на верхнем уровне также есть бесконечный цикл, то будет предпринята попытка заново создать сокет*/
		if (sockfd != -1) 
		{
			ESP_LOGE(TAG, "Shutting down socket and restarting...");
			shutdown(sockfd, 0);
			close(sockfd);

		}
	}
	vTaskDelete(xLCDTaskHandle);
	vQueueDelete(lcd_string_queue);
	vTaskDelete(NULL);
}


