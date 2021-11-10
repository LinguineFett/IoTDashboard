#include <ESP_Mail_Client.h>
#include <ESP_Mail_FS.h>
#include <SDK_Version_Common.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"

#define LED 0 // D3 on the Node MCU
#define LED2 4 // D2 on the NodeMCU
#define LED3 5 // D1 on the NodeMCU
#define ldrPin A0 // A0 on the NodeMCU

int light; // light intensity

#define SMTP_HOST "smtp.gmail.com" //SMTP gmail host.
#define SMTP_PORT 465 //SMTP gmail port.
#define AUTHOR_EMAIL "example@gmail.com" //Sender email.
#define AUTHOR_PASSWORD "password" //Sender email password.
#define RECIPIENT_EMAIL "example@gmail.com" //Just set this as the same as the sender email.

SMTPSession smtp;
DHTesp dht;
const char* ssid = "TP-Link_8856";
const char* password = "87973365";
//const char* ssid = "Merry Xmas";
//const char* password = "09475325323";

const char* mqtt_server = "192.168.0.104";

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
 

  if(topic=="room/light"){
    String messagein;
  
    for (int i = 0; i < length; i++) {
      //Serial.print((char)message[i]);
      messagein += (char)message[i];
    }

  
    if (messagein == "ON"){
      Serial.println("Light is ON");
      digitalWrite(LED, HIGH);
    }else if (messagein == "OFF") {
      Serial.println("Light is OFF");
      digitalWrite(LED, LOW);
    }
  }  

  if (topic == "room/photoresistor"){  // Press Enter when updating the threshold, and wait a few seconds
    String messagein;
  
    for (int i = 0; i < length; i++) {
      //Serial.print((char)message[i]);
      messagein += (char)message[i];
    }

    int value = messagein.toInt();

    // the higher the value of light is (photoresistor intensity), the darker it is. 
    if (value <= light){
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      Serial.println("LED off");
    }else if (value > light) {
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, HIGH);
      Serial.println("LED on");

      ESP_Mail_Session session;

      session.server.host_name = SMTP_HOST ;
      session.server.port = SMTP_PORT;
      session.login.email = AUTHOR_EMAIL;
      session.login.password = AUTHOR_PASSWORD;
      session.login.user_domain = "";

      /* Declare the message class */
      SMTP_Message message;
    
      message.sender.name = "Lights Alert";
      message.sender.email = AUTHOR_EMAIL;
      message.subject = "LED turned on";
      message.addRecipient("MQTT",RECIPIENT_EMAIL);
    
       //Send HTML message
      String htmlMsg = "<div style=\"color:#0000FF;\"><h1>Hello</h1><p>The light intensity is below the threshold. LED has been turned on.</p></div>";
      message.html.content = htmlMsg.c_str();
      message.html.content = htmlMsg.c_str();
      message.text.charSet = "us-ascii";
      message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit; 
    
      if (!smtp.connect(&session))
        return;
    
      if (!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());
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


void setup() {
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  dht.setup(5, DHTesp::DHT11); // D1 on the Node MCU
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(ldrPin, INPUT);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("vanieriot");

  float temp= dht.getTemperature();
  float hum= dht.getHumidity();
  light = analogRead(ldrPin);
    
    char tempArr [8];
    dtostrf(temp,6,2,tempArr);
    char humArr [8];
    dtostrf(hum,6,2,humArr);
    char lightArr [8];
    dtostrf(light,6,2,lightArr);
      
       client.publish("IoTlab/temperature", tempArr);
       client.publish("IoTlab/humidity", humArr);
       client.publish("IoTlab/photoresistor", lightArr);

  delay(5000);
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
