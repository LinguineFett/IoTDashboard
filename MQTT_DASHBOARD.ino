#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#define LED 0 // D3 on the Node MCU

DHTesp dht;
const char* ssid = "TP-Link_8856";
const char* password = "87973365";
//const char* ssid = "Merry Xmas";
//const char* password = "09475325323";

const char* mqtt_server = "192.168.0.104";

WiFiClient vanieriot;
PubSubClient client(vanieriot);


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


void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messagein;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messagein += (char)message[i];
  }

  if(topic=="room/light"){
    if (messagein == "ON"){
      Serial.println("Light is ON");
      digitalWrite(LED, HIGH);
    }else if (messagein == "OFF") {
      Serial.println("Light is OFF");
      digitalWrite(LED, LOW);
    }
  }  
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
 
    
   //  String clientId = "ESP8266Client-";
   // clientId += String(random(0xffff), HEX);
    // Attempt to connect
   // if (client.connect(clientId.c_str())) {
           if (client.connect("vanieriot")) {

      Serial.println("connected");  
      client.subscribe("room/light");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dht.setup(5, DHTesp::DHT11); // D1 on the Node MCU
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("vanieriot");

  float temp= dht.getTemperature();
  float hum= dht.getHumidity();
    
    char tempArr [8];
    dtostrf(temp,6,2,tempArr);
    char humArr [8];
    dtostrf(hum,6,2,humArr);
      
       client.publish("IoTlab/temperature", tempArr);
       client.publish("IoTlab/humidity", humArr);

  delay(5000);
  }
