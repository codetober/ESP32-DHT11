#include <PubSubClient.h>
#include<WiFi.h>
#include<dht11.h>

#define DHT11PIN 4

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

int status = WL_IDLE_STATUS;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

dht11 DHT;

float tempC = 0.0;
float cToFRate = 1.8;
float cToF32 = 32;
float humidity = 0.0;

void setup() {
  Serial.begin(115200);
  delay(10);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  delay(100);

  connectWifi();
  initMQTT();
}

void loop() {
  if(WiFi.status() != WL_CONNECTED){
    connectWifi();
  } else {
    if(!client.connected()){
      connectMQTT();
    } else {
      publishDHT11();
    }
  }
  client.loop();
  delay(300000);
}

void initMQTT(){
  Serial.println("Initializing MQTT");
  
  randomSeed(micros());
  client.setServer(mqtt_server, 1883);
  connectMQTT();
}

void connectMQTT(){
  
  while(!client.connected()){
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);
    if(client.connect(clientId.c_str())){
      Serial.println("Successfully connected MQTT");
    } else {
      Serial.println("Error!");
      Serial.println(client.state());
      delay(60000);
    }
  }
}

void publishDHT11(){
  int dhtState = DHT.read(DHT11PIN);

  if(dhtState == 0 || dhtState == -1){
    humidity = DHT.humidity;
    tempC = DHT.temperature;

    Serial.println(String(humidity).c_str());
    Serial.println(String(convertCtoF(tempC)).c_str());

    client.publish("codetober/humid", String(humidity).c_str());
    client.publish("codetober/tempF", String(convertCtoF(tempC)).c_str());
    client.publish("codetober/tempC", String(tempC).c_str());
  } else {
    Serial.println("Error fetching DHT11 Data");
  }
}

void connectWifi(){
  WiFi.begin(ssid, password);
  
  while(status != WL_CONNECTED){
    status = WiFi.status();
    delay(1000);
    Serial.print(".");
  }
}

float convertCtoF(float t){
  return ((t * cToFRate) + cToF32);
}
