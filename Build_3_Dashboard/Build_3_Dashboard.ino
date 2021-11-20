#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
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


// RFID
constexpr uint8_t RST_PIN = D3;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D4;     // Configurable, see typical pin layout above
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

#define BUZZER_PIN 5 // D1 on NodeMCU


//const char* ssid = "TP-Link_8856_G";
//const char* password = "87973365";
//const char* ssid = "Merry Xmas";
//const char* password = "09475325323";

const char* ssid = "Homeless";
const char* password = "Number11";
const char* mqtt_server = "10.0.0.30";

//const char* mqtt_server = "192.168.0.104";
//const char* ssid = "TP-Link_2AD8";
//const char* password = "14730078";

//const char* ssid = "BigBalls";
//const char* password = "Wagwan123";
//const char* mqtt_server = "192.168.166.161";

//const char* mqtt_server = "192.168.0.160";

WiFiClient vanieriot;
WiFiClient vanieriot2;
WiFiClient vanieriot3;
PubSubClient clientFan(vanieriot);
PubSubClient clientLight(vanieriot2);
PubSubClient clientRFID(vanieriot3);

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



String tag;
String user;

float userTemp;
int userLight;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  clientFan.setServer(mqtt_server, 1883);
  clientFan.setCallback(callback);
  clientLight.setServer(mqtt_server, 1883);
  clientLight.setCallback(callback);
  clientRFID.setServer(mqtt_server, 1883);
  clientRFID.setCallback(callback);
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

  // RFID
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  if (!clientFan.connected())
    reconnectFanClient();
    
  if(!clientFan.loop())
    clientFan.connect("vanieriot");



  if (!clientRFID.connected())
    reconnectRFIDClient();
    
  if(!clientRFID.loop())
    clientRFID.connect("vanieriot3");  

    

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
    //delay(2000);


    // RFID
  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  if (rfid.PICC_ReadCardSerial()) {
    Serial.println("Reading tag");
    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
    Serial.println(tag);

    if (tag == "21112995"){
      user = "Brandon";
      userTemp = 24;
      userLight = 1000;
    }else if (tag == "12345678"){ // another tag...
      user = "Other";
      userTemp = 20;
      userLight = 700;
    }else{
      Serial.println("Invalid user");
      digitalWrite(BUZZER_PIN, HIGH);
      delay(5000);
      digitalWrite(BUZZER_PIN, LOW);
    }

    int str_len = user.length()+1;
    char tag_array[str_len];
    user.toCharArray(tag_array, str_len);

    char usertempArr [8];
    dtostrf(userTemp,6,2,usertempArr);
    char userlightArr [8];
    dtostrf(userLight,6,2,userlightArr);

    clientRFID.publish("IoTlab/RFID", tag_array);
    clientRFID.publish("user/temperature", usertempArr);
    clientRFID.publish("user/light", userlightArr);

    tag = "";
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(2000);  
  }  
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

void reconnectRFIDClient() {
  while (!clientRFID.connected()) {
    Serial.print("Attempting MQTT connection...");
      if (clientRFID.connect("vanieriot3")) {
      Serial.println("connected RFID");  
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientRFID.state());
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
