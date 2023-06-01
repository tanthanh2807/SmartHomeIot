#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Servo.h>
#include "DHT.h"
#include <ESP_Mail_Client.h>
//#include <SimpleDHT.h>
#define Relay1            D5
#define Fan               D2
#define DHTPIN            D3
#define DHTTYPE DHT11
int Pump = 13;            //D7
Servo myServo;

#define WIFI_SSID       "IOTGhostRider"             // Your SSID
#define WIFI_PASS       "00112233445"        // Your password
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
/* The sign in credentials */
#define AUTHOR_EMAIL "dinhtanthanh001@gmail.com"
#define AUTHOR_PASSWORD "kghsbrnoymlpxtzi"
/* Recipient's email*/
#define RECIPIENT_EMAIL "cnweb19ce004@gmail.com"

/* The SMTP Session object used for Email sending */
SMTPSession smtp;
/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "ttanh19ce"            // Replace it with your username
#define AIO_KEY         "aio_vpzU74HPhDuPpawn6UjA8gZSsptD"   // Replace with your Project Auth Key
// DHT sensor
DHT dht(DHTPIN, DHTTYPE, 15);
/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/


// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe Light1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME"/feeds/Light"); // FeedName
Adafruit_MQTT_Subscribe Fan1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME"/feeds/Fan");
Adafruit_MQTT_Subscribe Door = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME"/feeds/Door");
Adafruit_MQTT_Publish Temperature1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME"/feeds/Temperature");
Adafruit_MQTT_Publish Humidity1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME"/feeds/Humidity");

//int pinDHT11 = 0; //D3
//SimpleDHT11 dht11(pinDHT11);
//byte hum = 0;
//byte temp = 0;
void MQTT_connect();
void smtpCallback(SMTP_Status status);
void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(D3,OUTPUT);
  pinMode(Relay1, OUTPUT);
  pinMode(Fan, OUTPUT);
  pinMode(Pump, OUTPUT);
  myServo.attach(D4);  // GPIO2 (D4)
 
  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());
 

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&Light1);
  mqtt.subscribe(&Fan1);
  mqtt.subscribe(&Door);
  
}

void loop() {
 
  MQTT_connect();
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &Light1) {
      Serial.print(F("Got: "));
      Serial.println((char *)Light1.lastread);
      int Light1_State = atoi((char *)Light1.lastread);
      digitalWrite(Relay1, !(Light1_State));
      Serial.println("Gia tri Light:");
      Serial.println(Light1_State);
      }
    if (subscription == &Fan1) {
      Serial.print(F("Got: "));
      Serial.println((char *)Fan1.lastread);
      int Fan1_State = atoi((char *)Fan1.lastread);
      digitalWrite(Fan, !(Fan1_State));
      }    
    if (subscription == &Door) {
      Serial.println("Cua chinh:");
      Serial.println((char*) Door.lastread);
      if (!strcmp((char*) Door.lastread, "ON"))
      {
       myServo.write(90);
    }
      if (!strcmp((char*) Door.lastread, "OFF"))
      {
       myServo.write(0);
    }
    }
  }
  int humidity_data = (int)dht.readHumidity();
  int temperature_data = (int)dht.readTemperature();
  // PUBLISH DATA
  if (! Temperature1.publish(temperature_data))
    Serial.println(F("Failed to publish temperature"));
  else
    Serial.println(F("Temperature published!"));

  if (! Humidity1.publish(humidity_data))
    Serial.println(F("Failed to publish humidity"));
  else
    Serial.println(F("Humidity published!"));
    delay(10000);
  int gas = digitalRead(D0);
  int Fire = digitalRead(D1);
  /** Enable the debug via Serial port
   * none debug or 0
   * basic debug or 1
  */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  /* Declare the message class */
  SMTP_Message messageGas;
  SMTP_Message messageFire;
  
  /* Set the message headers */
  if (gas == 1){
    Serial.println("Co gas");
    messageGas.sender.name = "SmartHome";
    messageGas.sender.email = AUTHOR_EMAIL;
    messageGas.subject = "Canh bao khi gas";
    messageGas.addRecipient("Sara", RECIPIENT_EMAIL);
    String textMsg = "Canh bao co khi gas trong nha";
    messageGas.text.content = textMsg.c_str();
    messageGas.text.charSet = "us-ascii";
    messageGas.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    messageGas.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    messageGas.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
    gas = 0;
    delay(1000);
  /* Connect to server with the session config */
    if (!smtp.connect(&session))
      return;

  /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &messageGas))
      Serial.println("Error sending Email, " + smtp.errorReason());
  }
  if (Fire == 1){
    Serial.println("Co lua, khoi");
    digitalWrite(Pump, HIGH);     // mo voi cuu hoa
    myServo.write(90);            // mo cua chinh
    messageFire.sender.name = "SmartHome";
    messageFire.sender.email = AUTHOR_EMAIL;
    messageFire.subject = "Canh bao chay";
    messageFire.addRecipient("Sara", RECIPIENT_EMAIL);
    String textMsg = "Canh bao co lua chay trong nha";
    messageFire.text.content = textMsg.c_str();
    messageFire.text.charSet = "us-ascii";
    messageFire.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    messageFire.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    messageFire.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
    Fire = 0;
    delay(1000);
  /* Connect to server with the session config */
    if (!smtp.connect(&session))
      return;

  /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &messageFire))
      Serial.println("Error sending Email, " + smtp.errorReason());
  }
  }
  
  


void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
  
}
/* Callback function to get the Email sending status */
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
