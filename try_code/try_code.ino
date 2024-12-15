// Define the pins
#define sensorPin A0
#define ledPin D4

// Variables to store sensor value
int sensorValue = 0;

void setup() {
  // Initialize serial communication at 9600 baud rate
  Serial.begin(9600);
  
  // Initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Read the analog value from the sensor
  sensorValue = analogRead(sensorPin);
  
  // Print the sensor value to the Serial Monitor
  Serial.print("Soil Moisture Value: ");
  Serial.println(sensorValue);
  
  // Check if the soil is dry
  if (sensorValue > 500) {
    // Turn the LED on
    Serial.println("se aprinde");
   // digitalWrite(ledPin, HIGH);
  } else {
    // Turn the LED off
    Serial.println("se stinge");
    //digitalWrite(ledPin, LOW);
  }
  
  // Wait for a second before taking another reading
  delay(1000);
}