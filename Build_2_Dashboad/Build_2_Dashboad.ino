#include <ESP_Mail_Client.h>
#include <ESP_Mail_FS.h>
#include <SDK_Version_Common.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

#include <SPI.h>
#include <MFRC522.h>

#include <NTPClient.h>
#include <WiFiUdp.h>


// LED and Photo Resistor
#define LED1 D0
#define LED2 D1

#define photoResistorPIN A0

// DHT11

#define dht11 D0
float temp; // Temperature level
float hum; // Humidity level
int light; // light intensity

//Motor
#define motor1 D1
#define motor2 D2
#define enable D3

#define SMTP_HOST "smtp.gmail.com" //SMTP gmail host.
#define SMTP_PORT 465 //SMTP gmail port.
#define AUTHOR_EMAIL "email@gmail.com" //Sender email.
#define AUTHOR_PASSWORD "password" //Sender email password.
#define RECIPIENT_EMAIL "email@gmail.com" //Just set this as the same as the sender email.

SMTPSession smtp;
DHTesp dht;
//const char* ssid = "TP-Link_8856_G";
//const char* password = "87973365";
//const char* ssid = "Merry Xmas";
//const char* password = "09475325323";

const char* ssid = "wifi";
const char* password = "password";

//const char* mqtt_server = "10.0.0.30";

//const char* mqtt_server = "192.168.0.104";
const char* mqtt_server = "ip"; 


constexpr uint8_t RST_PIN = D3;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D4;     // Configurable, see typical pin layout above

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

// Server time
//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "pool.ntp.org");

#define BUZZER_PIN 5 // D1 on NodeMCU

WiFiClient vanieriot;
PubSubClient client(vanieriot); 


void setup_email(){
   smtp.callback(smtpCallback);
   ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "ESP";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP Test Email";
  message.addRecipient("Admin", RECIPIENT_EMAIL);
  
  String textMsg = "The temperature is passed the threshold! Turn on LED?";
  message.text.content = textMsg.c_str();
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
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


void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  if (topic == "room/fan") {  // Press Enter when updating the threshold, and wait a few seconds
    String messagein;
  
    for (int i = 0; i < length; i++) {
      //Serial.print((char)message[i]);
      messagein += (char)message[i];
    }

    Serial.println(messagein);
    
    if(messagein == "ON") {
      analogWrite(enable, 200);
    } else {
      analogWrite(enable, 0);
    }
  }

//  if (topic == "room/photoresistor"){  // Press Enter when updating the threshold, and wait a few seconds
//    String messagein;
//  
//    for (int i = 0; i < length; i++) {
//      //Serial.print((char)message[i]);
//      messagein += (char)message[i];
//    }
//
//    int value = messagein.toInt();
//
//    // a high value for light (photoresistor intensity) means that the room is dark 
//    if (value <= light){ // too bright, so turn off LEDs
//      digitalWrite(LED1, LOW);
//      digitalWrite(LED2, LOW);
//      Serial.println("LED off");
//    }else if (value > light) { // too dark, so turn on LEDs
//      digitalWrite(LED1, HIGH);
//      digitalWrite(LED2, HIGH);
//      Serial.println("LED on");
//
//      ESP_Mail_Session session;
//
//      session.server.host_name = SMTP_HOST ;
//      session.server.port = SMTP_PORT;
//      session.login.email = AUTHOR_EMAIL;
//      session.login.password = AUTHOR_PASSWORD;
//      session.login.user_domain = "";
//
//      /* Declare the message class */
//      SMTP_Message message;
//    
//      message.sender.name = "Lights Alert";
//      message.sender.email = AUTHOR_EMAIL;
//      message.subject = "LED turned on";
//      message.addRecipient("MQTT",RECIPIENT_EMAIL);
//    
//       //Send HTML message
//      String htmlMsg = "<div style=\"color:#0000FF;\"><h1>Hello</h1><p>The light intensity is below the threshold. LED has been turned on.</p></div>";
//      message.html.content = htmlMsg.c_str();
//      message.html.content = htmlMsg.c_str();
//      message.text.charSet = "us-ascii";
//      message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit; 
//    
//      if (!smtp.connect(&session))
//        return;
//    
//      if (!MailClient.sendMail(&smtp, &message))
//        Serial.println("Error sending Email, " + smtp.errorReason());
//    }
//  }
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
      if (client.connect("vanieriot")) {

      Serial.println("connected");  
      client.subscribe("room/fan");
      client.subscribe("room/light");
      client.subscribe("room/photoresistor");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
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
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dht.setup(dht11, DHTesp::DHT11); // D0 on the Node MCU

  // LED and Photoresistor
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
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

  // time for the RFID email
  //timeClient.begin();
  //timeClient.setTimeOffset(0);
}

void loop() {
  //timeClient.update();

  //String formattedTime = timeClient.getFormattedTime();
  

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("vanieriot");

  
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

      digitalWrite(BUZZER_PIN, HIGH);
      delay(5000);
      digitalWrite(BUZZER_PIN, LOW);
    }else if (tag == "12345678"){ // anotther tag...
      user = "Other";
      userTemp = 20;
      userLight = 700;
    }else{
      Serial.println("Invalid user");
      digitalWrite(BUZZER_PIN, HIGH);
      delay(5000);
      digitalWrite(BUZZER_PIN, LOW);
    }
    

    if (user == "Brandon" || user == "Other"){
      ESP_Mail_Session session;

      session.server.host_name = SMTP_HOST ;
      session.server.port = SMTP_PORT;
      session.login.email = AUTHOR_EMAIL;
      session.login.password = AUTHOR_PASSWORD;
      session.login.user_domain = "";

      /* Declare the message class */
      SMTP_Message message;
    
      message.sender.name = "RFID tag alert";
      message.sender.email = AUTHOR_EMAIL;
      message.subject = "New login";
      message.addRecipient("MQTT",RECIPIENT_EMAIL);
    
       //Send HTML message
      String htmlMsg = "<div style=\"color:#0000FF;\"><h1>Hello</h1><p>User '" + user + "' was here.</p></div>";
      message.html.content = htmlMsg.c_str();
      message.html.content = htmlMsg.c_str();
      message.text.charSet = "us-ascii";
      message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit; 
    
      if (!smtp.connect(&session))
        return;
    
      if (!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());
    }

    int str_len = user.length()+1;
    char tag_array[str_len];
    user.toCharArray(tag_array, str_len);

    char usertempArr [8];
    dtostrf(userTemp,6,2,usertempArr);
    char userlightArr [8];
    dtostrf(userLight,6,2,userlightArr);

    

    temp = dht.getTemperature();
    hum = dht.getHumidity();
    light = analogRead(photoResistorPIN);

    if (userTemp < temp){ // room is hot
      // turn on fan...
    }
    if (userLight > light){ // room is dark
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      Serial.println("LED on");
      // not tested
    }


    client.publish("IoTlab/RFID", tag_array);
    client.publish("room/temperature", String(temp).c_str());
    client.publish("user/temperature", usertempArr);
    client.publish("user/light", userlightArr);
    client.publish("room/humidity", String(hum).c_str());
    //    client.publish("room/photoresistor", lightArr);

    tag = "";
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    //delay(3000);
  }
}
  
  void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
 }
