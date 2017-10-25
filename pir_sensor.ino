#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <DHT.h>
#define DHTPIN D5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
char hchar[2];
char fchar[5];

char ssid[] = "IoT";
char pass[] = "@rduin0!";
int wstatus = WL_IDLE_STATUS;

const char* mqtt_server = "192.168.100.6"; 
const char* statussub = "home/inside/livingroom/sensor/motion/status"; 
const char* tempsub = "home/inside/livingroom/sensor/temperature";
const char* humidsub = "home/inside/livingroom/sensor/humidity";
const char* clientname = "LRM Motion";

const int pirSensor = D7;
boolean sensorStatus = false;
unsigned long currentMilis;

WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP udp;
byte influx[] = {192, 168, 100, 7};
int port = 8888;


void callback(char* topic, byte* payload, unsigned int length) {
  for (int i=0;i<length;i++) {
    if (String(topic) == String(statussub))  {
      char receivedChar = (char)payload[i];
    }
  }
}
 
void reconnect() {
  if( client.connect(clientname)) {
  client.subscribe(statussub);
  client.subscribe(tempsub);
  client.subscribe(humidsub);
  }
}
 
void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  connectWifi();
  pinMode(pirSensor, INPUT);
  dht.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnect(); 
}

void connectWifi() {
  WiFi.begin(ssid, pass); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void checkTemp() {
    float h = dht.readHumidity();
    float f = dht.readTemperature(true);
    dtostrf(h, 2, 1, hchar);
    client.publish(humidsub, hchar, true);
    dtostrf(f, 4, 1, fchar);
    client.publish(tempsub, fchar, true);
    sendudp("living_room_temp value=" + String(f));
    sendudp("living_room_humidity value=" + String(h));
}

void sendudp(String poststring) {
  udp.beginPacket(influx, port);
  udp.print(poststring);
  udp.endPacket();
}

void checkSensor() {
  if (digitalRead(pirSensor) != sensorStatus) {
    if (digitalRead(pirSensor)) {
      client.publish(statussub, "1", true);
    } else {
      client.publish(statussub, "0", true);
    }
    sensorStatus = digitalRead(pirSensor);
  }
}

void loop() {
  if (millis() - currentMilis >= 10000 ) {
    checkTemp();
    if (WiFi.status() != WL_CONNECTED) {
      connectWifi();
    }
    currentMilis = millis();
  }
   
  if (millis() - currentMilis < 0) {
    currentMilis = millis();
  }

  if (!client.loop()) {
    reconnect();
  } else {
    client.loop();
  }
  checkSensor();
}
