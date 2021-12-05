#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include "DHTesp.h"

// LED and Photo Resistor
#define LED1 D7
#define LED2 D8
#define photoResistorPIN A0

// DHT11
#define dht11 D0
DHTesp dht;
float temp; // Temperature level
float hum; // Humidity level
int light; // light intensity

//Motor
#define motor1 D1
#define motor2 D2
#define enable D3

const char* ssid = "ENTER_WIFI_NAME_HERE";
const char* password = "ENTER_WIFI_PASSWORD_HERE";
const char* mqtt_server = "ENTER_WIFI_IP_ADDRESS_HERE";

//const char* ssid = "BigBalls";
//const char* password = "Wagwan123";
//const char* mqtt_server = "192.168.166.68";

WiFiClient vanieriot;
WiFiClient vanieriot2;
PubSubClient clientFan(vanieriot);
PubSubClient clientLight(vanieriot2);

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.println(". Message: ");

  if (topic == "room/fan") {  // Press Enter when updating the threshold, and wait a few seconds
    String messagein;
  
    for (int i = 0; i < length; i++) {
      //Serial.print((char)message[i]);
      messagein += (char)message[i];
    }
    
    Serial.println("Fan is ");
    Serial.println(messagein);
    if(messagein == "ON") {
      analogWrite(enable, 200);
    } else {
      analogWrite(enable, 0);
    }
  }
  if (topic == "room/photoresistorThreshold"){  // Press Enter when updating the threshold, and wait a few seconds
    String messagein;
  
    for (int i = 0; i < length; i++) {
      messagein += (char)message[i];
    }
    int threshold = messagein.toInt();

    // a high value for light (photoresistor intensity) means that the room is dark 
    if (light >= threshold){ // too bright, so turn off LEDs
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      clientLight.publish("room/led", "OFF");
    }else if (light < threshold) { // too dark, so turn on LEDs
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      clientLight.publish("room/led", "ON");
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  clientFan.setServer(mqtt_server, 1883);
  clientFan.setCallback(callback);
  clientLight.setServer(mqtt_server, 1883);
  clientLight.setCallback(callback);
  dht.setup(dht11, DHTesp::DHT11); // D0 on the Node MCU

  // LED and Photoresistor
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  pinMode(photoResistorPIN, INPUT);
  
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(enable, OUTPUT);
  digitalWrite(motor1, HIGH);
  digitalWrite(motor2, LOW);
  analogWrite(enable, 0);
}

void loop() {
  if (!clientFan.connected())
    reconnectFanClient();
    
  if(!clientFan.loop())
    clientFan.connect("vanieriot");

  if (!clientLight.connected())
    reconnectLightClient();
    
  if(!clientLight.loop())
    clientLight.connect("vanieriot2");
    
    temp = dht.getTemperature();
    hum = dht.getHumidity();
    light = analogRead(photoResistorPIN);

    clientFan.publish("room/temperature", String(temp).c_str());
    clientFan.publish("room/humidity", String(hum).c_str());
    clientLight.publish("room/photoresistor", String(light).c_str());
    delay(2000);
}

void reconnectFanClient() {
  while (!clientFan.connected()) {
    Serial.print("Attempting MQTT connection...");
      if (clientFan.connect("vanieriot")) {
      Serial.println("connected Fan");  
      clientFan.subscribe("room/fan");
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientFan.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void reconnectLightClient() {
  while (!clientLight.connected()) {
    Serial.print("Attempting MQTT connection...");
      if (clientLight.connect("vanieriot2")) {
      Serial.println("connected Light");  
      clientLight.subscribe("room/photoresistorThreshold");
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientLight.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP-8266 IP address: ");
  Serial.println(WiFi.localIP());
}
