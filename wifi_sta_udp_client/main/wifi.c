#include "wifi.h"
//-------------------------------------------------------------
static const char *TAG = "wifi";
static int s_retry_num = 0; // глобальная переменная для подсчёта количества попыток соединения
static EventGroupHandle_t s_wifi_event_group; // объявление глобальной переменной типа структуры для группы событий
#define WIFI_CONNECTED_BIT BIT0  //макрос для события попытки соединения
#define WIFI_FAIL_BIT      BIT1 //макрос для события неудачного соединения

//--------------------------------------функция-обработчик событий--------------------------------------//
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) //условие срабатывает только в момент старта WiFi
  {
      esp_wifi_connect();
  }
  else
  {
	  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) //если не удалось соединиться
	  {
		  if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) //если количество соединений не превысило максимально допустимое количество соединений
		  {
			  esp_wifi_connect(); //новая попытка соединения
			  s_retry_num++; //увеличение счётчика количества соединений
			  ESP_LOGI(TAG, "retry to connect to the AP");
		  }
		  else
		  {
			  xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT); //установка бита превышения счётчика количества соединений
		  }
		  ESP_LOGI(TAG,"connect to the AP fail");
	  }
	  else
	  {
		  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) //если произошло удачное соединение и получен адрес IP
		  {
			  ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data; //указатель на структуру события
			  ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
			  s_retry_num = 0; //сброс количества попыток соединения
			  xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); //установка бит в группе событий
			  gpio_set_level (CONFIG_BLINK_GPIO, 1);
		  }
	 }
  }
}
//-------------------------------------------------------------
void wifi_init_sta(void)
{
  s_wifi_event_group = xEventGroupCreate(); // создание группы событий
  //макрос ESP_ERROR_CHECK проверяет возврат из функции кода ошибки, и приостанавливает работу программы в случае ошибки
  ESP_ERROR_CHECK(esp_netif_init()); //инициализация базового стека TCP/IP
  ESP_ERROR_CHECK(esp_event_loop_create_default()); //создание цикла событий по умолчанию
  esp_netif_create_default_wifi_sta(); //User init default station

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //инициализация переменной cfg типа структуры управления настройками WiFi
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  //две переменные-идентификатора экземпляров зарегистрированных обработчиков событий
  esp_event_handler_instance_t instance_any_id; //обработка любого события
  esp_event_handler_instance_t instance_got_ip; //обработки события получения IP

  // регистрация обработчиков для событий
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &event_handler,
                                                      NULL,
                                                      &instance_got_ip));
  //объявление и инициализации переменной типа структуры для свойств станции, полям которой присвоены SSID и пароль, которые были добавлены в конфигураторе, и режим шифрования
  wifi_config_t wifi_config =
  {
      .sta =
      {
          .ssid = CONFIG_ESP_WIFI_SSID,
          .password = CONFIG_ESP_WIFI_PASSWORD,
     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
          .pmf_cfg =
          {
              .capable = true,
              .required = false
          },
      },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) ); //установка режима работы WiFi как станции
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); //Установка нормального типа энергосбережения Wi-Fi
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) ); //Применение конфигурационных установок для WiFi станции
  ESP_ERROR_CHECK(esp_wifi_start() ); //запуск WiFi станции

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  EventBits_t bits = xEventGroupWaitBits
		  (s_wifi_event_group,
          WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
          pdFALSE,
          pdFALSE,
          portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT)
  {
      ESP_LOGI(TAG, "connected to ap SSID:%s", CONFIG_ESP_WIFI_SSID);
  }
  else
  {
	  if (bits & WIFI_FAIL_BIT)
	  {
		  ESP_LOGI(TAG, "Failed to connect to SSID:%s", CONFIG_ESP_WIFI_SSID);
	  }
	  else
	  {
		  ESP_LOGE(TAG, "UNEXPECTED EVENT");
	  }
  }
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip)); // разрегистрация обработчиков для событий
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
  vEventGroupDelete(s_wifi_event_group); //удаление группы событий
}
//-------------------------------------------------------------
