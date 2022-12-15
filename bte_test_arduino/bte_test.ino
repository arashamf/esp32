#include "BluetoothSerial.h"
#include "string.h"

const unsigned int Command_Size = 6;
unsigned char count = 0;
const unsigned int LED = 5;
char incomingChar;
char buffer [10];

 
BluetoothSerial SerialBT; // Создаём экземпляр класса

//коллбэк сообщающй о соединение по ble клиента
void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  if(event == ESP_SPP_SRV_OPEN_EVT){
    Serial.println("Client Connected has address:");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", param->srv_open.rem_bda[i]); //адрес клиента
      if (i < 5) 
      {
        Serial.print(":");
      }
    }
    Serial.println(" ");
  }
}

 
void setup() {
  Serial.begin(57600); //настройка скорости уарта

  SerialBT.register_callback(callback);
  
  if (!SerialBT.begin("ESP32")) //имя отображаемое при соединении по BLE
  {
    Serial.println("An error occurred initializing Bluetooth");
  }
  else 
  {
    Serial.println("Bluetooth initialized");
  }

   pinMode(LED, OUTPUT);
   digitalWrite(LED, LOW);
}
 
void loop() {
  while (SerialBT.available()) //ф-ия получает количество байт(символов) доступных для чтения из интерфейса BLE
  {
    
    if (((buffer [count] = SerialBT.read()) == '\r') || (buffer [count] == '\n')) //если строка закончилась
    {
      buffer [count] = '\0';
      if ( buffer [0] != 0) //если это не пустая строка
        Serial.println (buffer);
      count = 0;
      if (strncmp ( buffer,"led_on", 6) == 0)
      {
        Serial.println("MODE_LED = ON");
        digitalWrite(LED, HIGH);
      }
      continue; 
    }
    count++; 
    if (count == 10)
    {
      count = 0;
      Serial.print (buffer);
      Serial.println (" - TooLongTextString");
    }
  }
  delay(10); 
}
