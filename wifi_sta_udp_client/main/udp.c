#include "udp.h"
//-------------------------------------------------------------
static const char *TAG = "udp";
extern xQueueHandle lcd_string_queue; //очередь передачи данных для lcd
portTickType xLastWakeTime; //переменая для работы с точной задержкой

//-----------------------------------------функция приёма пакетов с сервера-----------------------------------------//
static void recv_task(void *pvParameters)
{
	char buf[10] = {};
	int *sock = (int*) pvParameters;
	struct qLCDData xrcvLCDData;
	char str1[10];
	xrcvLCDData.y_pos = 0;
	xrcvLCDData.x_pos = 5;
	xrcvLCDData.str = str1;

	for(short i=0;;i++)
	{
		recv(*sock, buf, sizeof(buf), 0); //приём данных
	    snprintf(str1, sizeof(str1), "%6d", *(short*)buf);
	    xQueueSendToBack(lcd_string_queue, &xrcvLCDData, 0); //передача полученных UDP данных в очередь
	}
}

//----------------------------------------------------------------------------------------------------//
void udp_task(void *pvParameters)
{
	TaskHandle_t xRecvTask = NULL; //дескрипторы для задач
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;  //структуры с данными udp-сервера и udp-клиента

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0 ) //создание несвязанного сокета в домене связи и возвращает дескриптор файла, для использования в последующих вызовах
	{
	    ESP_LOGE(TAG, "socket not created\n");
	    vTaskDelete(NULL);
	}
	memset(&servaddr, 0, sizeof(servaddr)); //очистка памяти полей переменных типов структур и инициализации переменных для адреса клиента
	memset(&cliaddr, 0, sizeof(cliaddr));

	//--------------------------------------------Заполнение информации о клиенте--------------------------------------------//
	cliaddr.sin_family    = AF_INET;
	cliaddr.sin_addr.s_addr = INADDR_ANY;
	cliaddr.sin_port = htons(CONFIG_CLIENT_PORT);

	if (bind(sockfd, (const struct sockaddr *)&cliaddr,  sizeof(struct sockaddr_in)) < 0 )  //связывания сокета с адресом клиента
	{
	    ESP_LOGE(TAG, "socket not binded\n");
	    vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "socket was binded\n");

	//--------------------------------------------Заполнение информации о сервере--------------------------------------------//
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(CONFIG_SERVER_IP);
	servaddr.sin_port = htons(CONFIG_SERVER_PORT);

	xTaskCreate(recv_task, "recv_task", 4096, (void*)&sockfd, 5, &xRecvTask); //создание задачи прёма udp-пакетов
	xLastWakeTime = xTaskGetTickCount(); //Запись времени в переменную для задержки

	for(short i=0; i < 32767; i++) //отправка пакета с числом на сервер раз в 100 милисекунд
	{
	   sendto(sockfd, &i, 2,  0, (struct sockaddr*) &servaddr,  sizeof(servaddr));
	   vTaskDelayUntil( &xLastWakeTime, ( 100 / portTICK_RATE_MS ) );
	}

	shutdown(sockfd, 0);	//завершение соединения
	close(sockfd); 			//закрытие соединения
	vTaskDelete(xRecvTask);
	vTaskDelete(NULL);
}
//----------------------------------------------------------------------------------------------------//

