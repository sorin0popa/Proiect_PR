#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "env.h"


// Define component pins
#define sensor A0
#define waterPump D3
#define trigPin D8
#define echoPin D7

float duration;
float distance;
float value;
float fill_percentage;

// WiFi credentials
const char WIFI_SSID[] = "DESKTOP-DCI13GV 0351";
const char WIFI_PASSWORD[] = "B870d|65";

// const char WIFI_SSID[] = "Nimic's A55";
// const char WIFI_PASSWORD[] = "alex1111";


// Device name from AWS
const char THINGNAME[] = "PR_project";

// MQTT broker host address from AWS
const char MQTT_HOST[] = "a132pc9bxacqas-ats.iot.us-east-1.amazonaws.com";

// MQTT topics
const char AWS_IOT_PUBLISH_TOPIC1[] = "ESP8266/umiditate_sol";
const char AWS_IOT_PUBLISH_TOPIC2[] = "ESP8266/nivel_apa";
const char AWS_IOT_PUBLISH_TOPIC3[] = "ESP8266/stare_pompa";
const char AWS_IOT_SUBSCRIBE_TOPIC[] = "web_server/command";

// Publishing interval
long interval = 5000;

long low_interval = 1000;

// Timezone offset from UTC
const int8_t TIME_ZONE = +2;

// Last time message was sent
unsigned long lastMillis = 0;

// WiFiClientSecure object for secure communication
WiFiClientSecure net;

BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);

// MQTT client instance
PubSubClient client(net);

// Function to connect to NTP server and set time
void NTPConnect() {
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("done!");
}

char *getFormattedTime() {
  char* timeStr = (char*)malloc(30 * sizeof(char)); 

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    strftime(timeStr, 30, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return timeStr;
  }
  else {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  strftime(timeStr, 30, "%Y-%m-%d %H:%M:%S", timeinfo);
  return timeStr;
  }
}

// Callback function for message reception
void messageReceived(char *topic, byte *payload, unsigned int length) {

  if (payload[0] == '0')
  { 
  Serial.println("se opreste");
  String payload = "{\"Pump state\": 0}";
  client.publish(AWS_IOT_PUBLISH_TOPIC3, payload.c_str());

  delay(1000);
  
  digitalWrite(waterPump, HIGH); // OFF
  
    
  }
  else if (payload[0] == '1')
  {
    Serial.println("se porneste");
    String payload = "{\"Pump state\": 1}";
    client.publish(AWS_IOT_PUBLISH_TOPIC3, payload.c_str());

    delay(1000);

    digitalWrite(waterPump, LOW); // ON
  }
}


// Function to connect to AWS IoT Core
void connectAWS() {
  delay(3000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println(String("Attempting to connect to SSID: ") + String(WIFI_SSID));

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  // Connect to NTP server to set time
  NTPConnect();

  // Set CA and client certificate for secure communication
  net.setTrustAnchors(&cert);
  net.setClientRSACert(&client_crt, &key);

  // Connect MQTT client to AWS IoT Core
  client.setServer(MQTT_HOST, 8883);
  client.setCallback(messageReceived);

  Serial.println("Connecting to AWS IoT");

  // Attempt to connect to AWS IoT Core
  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(1000);
  }

  // Check if connection is successful
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to MQTT topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}

// Function to publish message to AWS IoT Core
void publishSoilMoisture() {
  char *time = getFormattedTime();
  StaticJsonDocument<200> doc;
  doc["Time"] = getFormattedTime();

  Serial.println(value);
  doc["Soil moisture humidity"] = value;

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC1, jsonBuffer);
}

// Function to publish message to AWS IoT Core
void publishWaterLevel() {
  StaticJsonDocument<200> doc;
  doc["Time"] = getFormattedTime();
  doc["Water level"] = fill_percentage;

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC2, jsonBuffer);
}

// Function to publish message to AWS IoT Core
void publishPumpState() {
  int waterPump = 1 - digitalRead(waterPump); // Citește starea pinului D3

  StaticJsonDocument<200> doc;
  doc["Time"] = getFormattedTime();
  doc["Pump state"] = 1 - digitalRead(waterPump);

  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  client.publish(AWS_IOT_PUBLISH_TOPIC3, jsonBuffer);
}

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  Serial.begin(9600);
  
  pinMode(waterPump, OUTPUT);
  digitalWrite(waterPump, HIGH); // e pe off

  // Connect to AWS IoT Core
  connectAWS();
}

// Get the soil moisture values
float soilMoistureSensor() {
  float value = analogRead(sensor);
  value = map(value, 0, 1024, 0, 100);
  value = (value - 100) * -1;

  return value;
}


void loop() {
  // Check if it's time to publish a message
  if (millis() - lastMillis > interval) {
    lastMillis = millis();
   
    value = soilMoistureSensor();

     // Clears the trigPin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculating the distance
    distance = duration * 0.034 / 2;

 // Replace invalid readings with 0
  if (distance < 1.5) {
    distance = 0; 
  }
    if (distance > 12) {
    distance = 12;
  }
    // Calcul procentaj umplere
    fill_percentage = map(distance, 0, 12, 100, 0);

    if (client.connected()) {
      // Publish messages
       publishSoilMoisture();
       publishWaterLevel();
       publishPumpState();
    }
    
  }
  // Maintain MQTT connection
  client.loop();
}