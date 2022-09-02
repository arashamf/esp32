#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

#define LEDpin 23

const char* ssid = "gamma-negotiation";
const char* password =  "Olimpionik";
WebServer server(80);

void handleRoot();
void handleNotFound();

uint8_t count = 12;
bool LEDstatus = LOW;

void setup() 
{
   
  Serial.begin(115200);
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, LOW);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); //подключение к сети

  Serial.print("Connecting to WiFi");
  while ((WiFi.status() != WL_CONNECTED) && (count--))
  {
    delay(500);
    Serial.print(".");
  }
   Serial.println("");
  if (count != 0)
  {
    Serial.println("Connected to the WiFi network");
    Serial.print("IP address: "); Serial.println(WiFi.localIP());    
  }
  else
  {
    Serial.println("Don't connected to the WiFi network");
  }
  
  server.on("/", handleRoot);
  server.on("/ledon", handleLedOn);
  server.on("/ledoff", handleLedOff);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop() {
   
  server.handleClient();
}

void handleRoot() 
{
  server.send(200, "text/plain", "hello from esp32!");
}

void handleLedOn() 
{
  server.send(200, "text/plain", "led_on!");
  digitalWrite(LEDpin, HIGH);
}

void handleLedOff() 
{
  server.send(200, "text/plain", "led_off!");
   digitalWrite(LEDpin, LOW);
}

void handleNotFound() 
{
  server.send(404, "text/plain", "File Not Found");
}

/*void loop() 
{
  WiFiClient client = server.available(); // прослушка входящих клиентов
  if (client) // если подключается новый клиент
  { 
    Serial.println("New Client."); // выводим сообщение
    currentLine = "";
    while (client.connected())  // цикл, пока есть соединение клиента
    {
      if (client.available()) // если от клиента поступают данные,
      { 
        char c = client.read(); // читаем байт, затем
        Serial.write(c); // выводим на экран
        header += c;
        if (c == '\n') // если байт является переводом строки
        { 
          // если пустая строка, мы получили два символа перевода строки, значит это конец HTTP-запроса, формируем ответ сервера:
          if (currentLine.length() == 0) 
          {
            // HTTP заголовки начинаются с кода ответа (HTTP / 1.1 200 OK) и content-type, затем пустая строка:           
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
             // Включаем или выключаем светодиоды
            if (header.indexOf("GET /ledon") >= 0) 
            {
              Serial.println("led on");
              led_state = "on";
              digitalWrite(LEDpin, HIGH);
            } 
            else 
            {
              if (header.indexOf("GET /ledoff") >= 0) 
              {
                Serial.println("led off");
                led_state = "off";
                digitalWrite(LEDpin, LOW);
              } 
            }
            // Формируем веб-страницу на сервере
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS для кнопок можете менять под свои нужды
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border:  none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color:#555555;}</style></head>");
            client.println("<body><h1>ESP32 Web Server</h1>");
            // Выводим текущее состояние кнопок
            client.println("<p>GPIO 26 - State " +  led_state + "</p>");
            // Если led сейчас off, то выводим надпись ON
            if (led_state == "off") 
            {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } 
            else 
            {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            // HTTP-ответ завершается пустой строкой
            client.println();
            break;
          } 
          else // если получили новую строку, очищаем currentLine
          { 
            currentLine = "";
          }
        } 
        else 
        {
          if (c != '\r') // Если получили что-то ещё кроме возврата строки,
          { 
            currentLine += c; // добавляем в конец currentLine
          }
        }               
      }      
    }    
    header = "";  // Очистим переменную
    client.stop(); // Закрываем соединение
    Serial.println("Client disconnected.");
    Serial.println(""); 
  }     
}*/
