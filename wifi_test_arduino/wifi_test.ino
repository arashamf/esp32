#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WebServer.h>

#define LEDpin 23

const char* ssid = "gamma-negotiation";
const char* password =  "Olimpionik";

bool LEDstatus = LOW;
uint8_t count = 10;
void setup() 
{
  Serial.begin(115200);
  pinMode(LEDpin, OUTPUT);
  WiFi.begin(ssid, password);

  while ((WiFi.status() != WL_CONNECTED) && (--count))
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  if (count != 0)
  {
    Serial.println("Connected to the WiFi network");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
     Serial.println("Don't connected to the WiFi network");
  }
} 
 
void loop() 
{
  if(LEDstatus)
  {
    digitalWrite(LEDpin, HIGH);
    LEDstatus = LOW;
  }
  else
  {
    digitalWrite(LEDpin, LOW);
    LEDstatus = HIGH;
  }
  delay(500);
}
