
#include <PubSubClient.h>
#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include <WiFi.h>
#include "esp_sleep.h"
#include <dht11.h>

#define DHT11PIN 4

const char* ssid = "<SSID_GOES_HERE>";
const char* password = "<PASSWORD_GOES_HERE>";
const char* mqtt_server = "<MQTT_SERVER>";

// 20 seconds
const int sleep_time_micro_seconds = 20000000;

int status = WL_IDLE_STATUS;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// DHT11 Variables
dht11 DHT11;
float humidity = 0.0;
float tempC = 0.0;
float cToFRate = 1.8;
float cToF32 = 32.0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(10);
  adc_power_on();

  WiFi.mode(WIFI_STA);
  delay(100);
  // Connect WiFi using SSID and Password
  connectWifi();
  initMqtt();
}

void loop() {
  // Check WiFi Connection
  if(status != WL_CONNECTED){
    connectWifi();
  } else {
    // Connected to Wifi, check MQTT
    if(!client.connected()){
      connectMqtt();
      publishDHT();
    } else {
      publishDHT();
    }
  }
  client.loop();
  // short delay to allow pubsubclient and tcp client to flush data
  delay(5000);
  
  goToSleep();
}


void disableWifi(){
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

void connectWifi(){
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);

  while(status != WL_CONNECTED){
    status = WiFi.status();
    delay(1000);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void initMqtt(){
  Serial.println();
  randomSeed(micros());
  client.setServer(mqtt_server, 1883);
  // setup callback function for subscriptions
  //...
  
  // connect to mqtt
  connectMqtt();
}

void connectMqtt(){
  while(!client.connected()){
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);
    if(client.connect(clientId.c_str())){
      Serial.println("Successfully Connected to MQTT");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("Going to Sleep and will try again later");
      goToSleep();
    }
  }
}

void publishDHT(){
  Serial.println("Gathering Data...");
  // Check status of the sensor
  int c = DHT11.read(DHT11PIN);
  if(c == 0 || c == -1){
    client.publish("esp32s/dht11/humidity", String(DHT11.humidity).c_str());
    client.publish("esp32s/dht11/tempc", String(DHT11.temperature).c_str());
    client.publish("esp32s/dht11/tempf", String(convertCToF(DHT11.temperature)).c_str());
  } else if(c == -2){
    Serial.println("DHT11, Timeout Error - might be disconnected");
  }
}

void printStringToSerial(float h, float t){
  Serial.print("Humidity % ");
  Serial.println((float) h, 2);
  Serial.print("Temp (F) ");
  Serial.println((float) convertCToF(t), 2);
  Serial.print("Temp (C) ");
  Serial.println((float) t, 2);
}

float convertCToF(float t){
  return ((t * cToFRate) + cToF32);
}

void goToSleep(){
  Serial.println("Going to sleep");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  adc_power_off();
  esp_wifi_stop();
  esp_bt_controller_disable();
  esp_sleep_enable_timer_wakeup(sleep_time_micro_seconds);
  esp_deep_sleep_start();
}
