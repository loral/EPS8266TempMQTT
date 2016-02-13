/* ESP8266 + MQTT Garage Door State and Temperature Node
   Can also receive commands; adjust messageReceived() function
   Modified from a sketch from MakeUseOf.com
   Written by: Loral Godfrey, Jan 16, 2016
*/

#include <MQTTClient.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// MQTT
const char* ssid = "ssid";
const char* password = "password";

String subscribeTopic = "openhab/garage/esp8266"; // subscribe to this topic; anything sent here will be passed into the messageReceived function
String tempTopic = "openhab/garage/temperature"; //topic to publish temperatures readings to
String doorTopic = "openhab/garage/door"; // topic to publish door state to
const char* server = "ip address"; // server or URL of MQTT broker
String clientName = "ESP8266"; // just a name used to talk to MQTT broker

WiFiClient wifiClient;
MQTTClient client;

String dPayload;
String tPayload;

// Reed Swtich
const int reedSwitch = 5; // brown black orange 10 KOhm resistor
int reedStatus = 0;
int oldReedStatus = 0;

// Temperatrue
#define ONE_WIRE_BUS 12 // Data wire is plugged into pin 12 on the ESP8266
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float t;
unsigned long previousMillis = 0;
const long getTempInterval = 60000; // in milliseconds
unsigned long currentMillis = millis();

// Create custon name for MQTT using mac address
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void setup() {
  Serial.begin(115200);
  client.begin(server, wifiClient);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Generate client name based on MAC address and last 8 bits of microsecond counter
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);

  Serial.print("Connecting to ");
  Serial.print(server);
  Serial.print(" as ");
  Serial.println(clientName);

  if (client.connect((char*) clientName.c_str())) {
    Serial.println("Connected to MQTT broker");
    Serial.print("Subscribed to: ");
    Serial.println(subscribeTopic);
    client.subscribe(subscribeTopic);
  }
  else {
    Serial.println("MQTT connect failed");
    Serial.println("Will reset and try again...");
    abort();
  }

  // Dallas sensor startup
  sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement

  // set pin mode for reed switch
  pinMode(reedSwitch, INPUT);
}

void loop() {
  reedStatus = digitalRead(reedSwitch);
  if ((oldReedStatus == LOW && reedStatus == HIGH) || (oldReedStatus == HIGH && reedStatus == LOW))
  {
    oldReedStatus = reedStatus;
    if (!client.connected()) {}
    else
    {
      if (reedStatus == HIGH)
      {
        dPayload = "CLOSED";
      }
      else
      {
        dPayload = "OPEN";
      }
      //Serial.println(dPayload);
      client.publish(doorTopic, dPayload);
      //Serial.println("published door state");
    }
  }

  currentMillis = millis();
  if (currentMillis - previousMillis >= getTempInterval)
  {
    previousMillis = currentMillis;
    sensors.requestTemperatures(); // Send the command to get temperatures
    //t = sensors.getTempCByIndex(0);
    t = sensors.getTempFByIndex(0);
    if (isnan(t) || !client.connected()) {}
    else {
      //tPayload = String((t * 1.8 + 32), 2); // convert C to F
      tPayload = String(t,2);
      //Serial.println(tPayload);
      client.publish(tempTopic, tPayload);
      //Serial.println("published temperatrue");
    }
  }
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();
}
