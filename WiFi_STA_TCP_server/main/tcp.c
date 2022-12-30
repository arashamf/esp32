#include "tcp.h"

//-----------------------------------------------------------------------------------------------------------------------------//
static const char *TAG = "tcp";
const unsigned char lenght = 16;
//-----------------------------------структура для работы с сокетом подключившегося клиента-----------------------------------//
typedef struct struct_client_socket_t
{
  struct sockaddr_in cliaddr;
  socklen_t sockaddrsize;
  int accept_sock;
  uint16_t y_pos;
} struct_client_socket;

struct_client_socket client_socket01;

//-----------------------------------------------------------------------------------------------------------------------------//
typedef struct
{
  unsigned char y_pos;
  unsigned char x_pos;
  char *str;
} qLCDData;

xQueueHandle lcd_string_queue = NULL;
xQueueHandle xQueueClose = NULL;
xQueueHandle xQueueCloseAsk = NULL;

//-----------------------------------------------------------------------------------------------------------------------------//
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

//------------------------задача, которая будет заниматься отдельным клиентом, подключенным к серверу------------------------//
static void client_socket_task(void *pvParameters)
{
	char buffer_string [lenght];
	int ret, accept_sock;
	qLCDData xLCDData;
	struct_client_socket *arg_client_socket;

	arg_client_socket = (struct_client_socket*) pvParameters; //присвоение адреса параметров задачи объявленному указателю
	struct sockaddr_in cliaddr; //переменная структуры адреса сокета
	socklen_t sockaddrsize; // размера данного адреса

	int buflen = lenght; //переменная длины буфера
	char data_buffer[16] = {}; //Объявление и инициализация массива для хранения данных буфера
	xLCDData.y_pos = arg_client_socket->y_pos; //инициализация поля переменной структуры для задачи дисплея
	xLCDData.str = buffer_string; //Присвоение адреса строки массива в структуре дисплея

	accept_sock = arg_client_socket->accept_sock; //Присвоение идентификатора сокета локальной переменной
	cliaddr = arg_client_socket->cliaddr;
	sockaddrsize = arg_client_socket->sockaddrsize;

	while(1) // цикл, в котором пытаемся принять пакет от клиента
	{
		ret = recvfrom( accept_sock, data_buffer, buflen, 0, (struct sockaddr *)&cliaddr, &sockaddrsize);

		if(ret > 0) //установка символа \0 в конец сообщения
		{
			data_buffer[ret] = 0;
			ESP_LOGI(TAG, "ret = %d", ret);
		}
		if(strncmp(data_buffer, "-c", 2) == 0) //Если пришел пакет с определённой строкой, которая будет служить командой разрыва соединения от клиента, то  сокет закрывается и задача удаляется
		{
			close(accept_sock);
			sprintf (buffer_string, "disonnected");
			xQueueSendToBack(lcd_string_queue, &xLCDData, 0);
			vTaskDelete(NULL);
		}
		//если пришел непустой пакет, то скопируем данные из буфера в строковую переменную, остаток до 20 забьём пробелами,
		//завершим нулём и отправим на дисплей в нужную позицию, соответствующую идентификатору (номеру) сокета. Также отправим строку с данными
		//в терминальную программу и обратно клиенту
	    if (strcmp (data_buffer, "\r\n") != 0) //если сообщение не пустое
	    {
	    	for(unsigned char count=0; count < ret; count++)
	    	{
	    		if ((data_buffer[count] == '\r') || (data_buffer[count] == '\n'))
	    			data_buffer[count]=' ';
	    	}
	    	strcpy(buffer_string, data_buffer);
	    	for(unsigned char count=ret; count<16; count++)
	    	{
	    		buffer_string[count]=' ';
	    	}
	    	buffer_string[15] = 0;
	    	xQueueSendToBack(lcd_string_queue, &xLCDData, 0);
	    	strcat(data_buffer,"\r\n");
	    	ESP_LOGI(TAG, "Socket %d: %s", accept_sock, data_buffer);
	    	sendto(accept_sock,data_buffer,strlen((char*)data_buffer),0,(struct sockaddr *)&cliaddr, sockaddrsize);
	    }
	}
}

//--------------------------------------------------------------------------------------------------------------------------//
void tcp_task(void *pvParameters)
{
	int sockfd;
	int accept_sock; // переменная для идентификации сокета, который будет использоваться для клиента, пытающегося соединиться с сервером
	struct sockaddr_in servaddr, cliaddr;  //структуры с данными udp-сервера и udp-клиента
	socklen_t sockaddrsize; //переменнаяю для хранения размера адреса сокета
	unsigned char y_pos = 0; //переменная для горизонтальной позиции на дисплее

	TaskHandle_t xLCDTaskHandle = NULL;
	qLCDData xLCDData;
	char lcd_buffer[16];
	xLCDData.x_pos = 2;
	xLCDData.y_pos = 0;
	xLCDData.str = lcd_buffer;
	lcd_string_queue = xQueueCreate(10, sizeof(qLCDData));
	xTaskCreate(vLCDTask, "vLCDTask", 2048, NULL, 2, &xLCDTaskHandle);
	xQueueClose = xQueueCreate(10, sizeof(unsigned char));
	xQueueCloseAsk = xQueueCreate(10, sizeof(unsigned char));
	ESP_LOGI(TAG, "Create socket...\n");

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0 ) //протокол tcp
	{
		ESP_LOGE(TAG, "socket not created\n");
		vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "Socket created");
	memset(&servaddr, 0, sizeof(servaddr)); //Очистка памяти полей переменных типов структур

	//Заполнение информации о сервере
	servaddr.sin_family    = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY; //данный адрес может быть разным
	servaddr.sin_port = htons(CONFIG_SERVER_PORT);
	
	//Свяжем сокет с адресом сервером
	if (bind(sockfd, (const struct sockaddr *)&servaddr,  sizeof(struct sockaddr_in)) < 0 )
	{
		ESP_LOGE(TAG, "socket not binded\n");
		vTaskDelete(xLCDTaskHandle);
	  	vQueueDelete(lcd_string_queue);
		vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "socket was binded\n");
	listen(sockfd, 5); //После связи сокета с адресом сервера, начинается прослушивание сокета (максимальное количество попыток соединений - 5)

	//бесконечный цикл, в котором ожидается подключения клиента, пока клиент не подключится, программа будем висеть в этом месте
	while(1)
	{
		memset(&cliaddr, 0, sizeof(cliaddr));
	    accept_sock = accept (sockfd, (struct sockaddr *)&cliaddr, (socklen_t *)&sockaddrsize);
	    printf("socket: %d\n", accept_sock); //значение переменной идентификатора сокета

	    //инициализация поля переменной и создание задачи, передав в качестве параметра переменную	с данными клиента     
	    if(accept_sock >= 0)
	    {
	      client_socket01.accept_sock = accept_sock;
	      client_socket01.cliaddr = cliaddr;
	      client_socket01.sockaddrsize = sockaddrsize;
	      client_socket01.y_pos = y_pos%4;
	      xTaskCreate(client_socket_task, "client_socket_task", 4096, (void*)&client_socket01, 5, NULL); // создание задачи, передав в качестве параметра переменную структуру с данными клиента
	    }
	 }

	 close (sockfd);
	 sprintf(lcd_buffer, "Disonnected");
	 xQueueSendToBack(lcd_string_queue, &xLCDData, 0);
	 vTaskDelay(2000 / portTICK_RATE_MS);
	 vTaskDelete(xLCDTaskHandle);
	 vQueueDelete(lcd_string_queue);
	 vTaskDelete(NULL);
}


