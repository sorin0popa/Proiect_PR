#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "env.h"


bool Relay = 0;

//Define component pins
#define sensor A0
#define waterPump D3
#define trigPin D8
#define echoPin D7

float duration;
float distance;
float value;

// WiFi credentials
const char WIFI_SSID[] = "DESKTOP-DCI13GV 0351";
const char WIFI_PASSWORD[] = "B870d|65";

// Device name from AWS
const char THINGNAME[] = "PR_project";

// MQTT broker host address from AWS
const char MQTT_HOST[] = "a132pc9bxacqas-ats.iot.us-east-1.amazonaws.com";

// MQTT topics
const char AWS_IOT_PUBLISH_TOPIC1[] = "esp8266/umiditate_sol";
const char AWS_IOT_PUBLISH_TOPIC2[] = "esp8266/nivel_apa";
const char AWS_IOT_SUBSCRIBE_TOPIC[] = "web_server/command";

// Publishing interval
const long interval = 5000;

// Timezone offset from UTC
const int8_t TIME_ZONE = -5;

// Last time message was sent
unsigned long lastMillis = 0;

// WiFiClientSecure object for secure communication
WiFiClientSecure net;

// X.509 certificate for the CA
BearSSL::X509List cert(cacert);

// X.509 certificate for the client
BearSSL::X509List client_crt(client_cert);

// RSA private key
BearSSL::PrivateKey key(privkey);

// MQTT client instance
PubSubClient client(net);

// Function to connect to NTP server and set time
void NTPConnect() {
  // Set time using SNTP
  Serial.print("Setting time using SNTP");
  configTime(TIME_ZONE * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 1510592825) { // January 13, 2018
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("done!");
}

// Callback function for message reception
void messageReceived(char *topic, byte *payload, unsigned int length) {
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (payload[0] == '0')
  { 
    Serial.println("se opreste");
    digitalWrite(waterPump, HIGH); // e pe off
  }
  else if (payload[0] == '1')
  {
    Serial.println("se porneste");
    digitalWrite(waterPump, LOW); // e pe on
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
  Serial.print("Moisture :");
  Serial.print(value);
  Serial.println(" ");

  // Create JSON document for message
  StaticJsonDocument<200> doc;
  doc["Time"] = time(nullptr);
  doc["Soil moisture humidity"] = value;

  // Serialize JSON document to string
  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  // Publish message to MQTT topic
  client.publish(AWS_IOT_PUBLISH_TOPIC1, jsonBuffer);
}

// Function to publish message to AWS IoT Core
void publishWaterLevel() {
  Serial.print("WaterLevel:");
  Serial.print(distance);
  Serial.println(" ");

  // Create JSON document for message
  StaticJsonDocument<200> doc;
  doc["Time"] = time(nullptr);
  doc["Water level"] = distance;

  // Serialize JSON document to string
  char jsonBuffer[200];
  serializeJson(doc, jsonBuffer);

  // Publish message to MQTT topic
  client.publish(AWS_IOT_PUBLISH_TOPIC2, jsonBuffer);
}

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  // Initialize serial communication
  Serial.begin(9600);
  
  pinMode(waterPump, OUTPUT);
  digitalWrite(waterPump, HIGH); // e pe off

  // Connect to AWS IoT Core
  connectAWS();
}

//Get the soil moisture values
float soilMoistureSensor() {
  float value = analogRead(sensor);
  Serial.print("[FUNC1] Moisture :");
  Serial.print(value);
  Serial.print(" ");

  value = map(value, 0, 1024, 0, 100);
  value = (value - 100) * -1;

  Serial.print("[FUNC2] Moisture :");
  Serial.print(value);
  Serial.println(" ");

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

    if (distance < 2) {
    distance = 0; // Replace invalid readings with 0
  }
    // Prints the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.println(distance);

    if (client.connected()) {
      // Publish message
       publishSoilMoisture();
       publishWaterLevel();
    }
    
  }
  // Maintain MQTT connection
  client.loop();
}