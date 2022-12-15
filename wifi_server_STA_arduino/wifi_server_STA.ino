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
