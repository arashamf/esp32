#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WebServer.h>

#define LEDpin 23

// Установите здесь свои SSID и пароль 
const char* ssid = "ESP32";  // Enter SSID here
const char* password = "12345678";  //Enter Password here

IPAddress local_ip(192,168,5,100);
IPAddress gateway(192,168,5,100);
IPAddress subnet(255,255,255,0);

WebServer server(80); //установка порта

bool LEDstatus = LOW;

void setup() 
{
  Serial.begin(115200);
  pinMode(LEDpin, OUTPUT);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);  //проверка SSID, пароль, IP-адреса, маски IP-подсети и IP-шлюза
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  //WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  //Чтобы обрабатывать входящие HTTP-запросы, нужно указать, какой код выполнять при вызове конкретного URL. Для этого спользуем метод on . Этот метод принимает два параметра. Первый - это URL-путь, а второй - имя функции,
 // которую необходимо выполнить при переходе по этому URL-адресу. Первая строка приведенного ниже кода указывает, что, когда сервер получает HTTP-запрос по корневому (/) пути, он запускает handle_OnConnect() функцию. 
 // Указанный URL-адрес является относительным путем
  server.on("/", handle_OnConnect);
  server.on("/ledon", handle_ledon);
  server.on("/ledoff", handle_ledoff);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
} 
 
void loop() 
{
  server.handleClient(); //метод объекта сервера
 /* if(LEDstatus)
  {digitalWrite(LEDpin, HIGH);}
  else
  {digitalWrite(LEDpin, LOW);}*/
}

void handle_OnConnect() 
{
  server.send(200, "text/plain", "hello!");
 // LEDstatus = LOW;
//  Serial.println("GPIO4 Status: OFF | GPIO5 Status: OFF");
//  server.send(200, "text/html", SendHTML(LEDstatus)); 
}

void handle_ledon() {
  LEDstatus = HIGH;
  Serial.println("Led_ Status: ON");
  digitalWrite(LEDpin, HIGH);
  server.send(200, "text/plain", "Led_ Status: ON");
 // server.send(200, "text/html", SendHTML(true)); 
}

void handle_ledoff() {
  LEDstatus = LOW;
  Serial.println("Led_Status: OFF");
  digitalWrite(LEDpin, LOW);
  server.send(200, "text/plain", "Led_ Status: OFF");
  //server.send(200, "text/html", SendHTML(false)); 
}

void handle_NotFound()
{
 server.send(404, "text/plain", "Not found");
}

/*String SendHTML(uint8_t ledstat)
{
  String ptr = "<!DOCTYPE html> <html>\n"; //Первый текст, который всегда должен быть отправлен это <! DOCTYPE>, который указывает на то, что мы посылаем HTML код
  ptr +="<meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr +="<title>Управление светодиодом</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-on {background-color: #3498db;}\n";
  ptr +=".button-on:active {background-color: #2980b9;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="p {font-size: 14px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>ESP32 Веб сервер</h1>\n";
  ptr +="<h3>Режим точка доступа WiFi (AP)</h3>\n";
  if(ledstat)
  {ptr +="<p>Состояние LED: ВКЛ.</p><a class=\"button button-off\" href=\"/ledoff\">ВЫКЛ.</a>\n";}
  else
  {ptr +="<p>Состояние LED: ВЫКЛ.</p><a class=\"button button-on\" href=\"/ledon\">ВКЛ.</a>\n";}
  ptr +="</body>\n";
  ptr +="</html>\n";
  return ptr;
}*/
