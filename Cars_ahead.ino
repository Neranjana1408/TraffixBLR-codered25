#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <math.h> // Required for mathematical functions

// nRF24L01 Setup
RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";     // Address for incoming
const byte ackAddress[6] = "00002";  // Address for outgoing acknowledgment

// GPS Setup
static const int RXPin = 3, TXPin = 4; // GPS RX → Arduino Pin 3, GPS TX → Arduino Pin 4
static const uint32_t GPSBaud = 9600; // GPS module baud rate
SoftwareSerial gpsSerial(RXPin, TXPin); // Create SoftwareSerial for GPS
TinyGPSPlus gps; // TinyGPS++ object for parsing GPS data

// Structure to receive GPS data from nRF24L01
struct Location {
  float latitude;   // Latitude
  float longitude;  // Longitude
  float speed;      // Speed in cm/s
};

// Variables to store received and local GPS coordinates
Location receivedData;
float localLatitude = 0.0, localLongitude = 0.0, localSpeed = 0.0;
bool receivedNewData = false; // Flag to indicate new data received

// LED Pins
const int msgReceivedLed = 5;
const int dangerLed = 6;

// Function to calculate distance using the Haversine formula
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371.0; // Earth's radius in kilometers
  double latRad1 = radians(lat1);
  double latRad2 = radians(lat2);
  double deltaLat = radians(lat2 - lat1);
  double deltaLon = radians(lon2 - lon1);

  double a = sin(deltaLat / 2) * sin(deltaLat / 2) +
             cos(latRad1) * cos(latRad2) *
             sin(deltaLon / 2) * sin(deltaLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c * 1000; // Distance in meters
}

void setup() {
  Serial.begin(9600);

  // Initialize radio
  radio.begin();
  radio.openReadingPipe(0, address);    // Listen for incoming data
  radio.openWritingPipe(ackAddress);   // Send acknowledgment to the transmitter
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(false); // Disable Auto-ACK
  radio.startListening();

  // GPS Initialization
  gpsSerial.begin(GPSBaud);

  // LED Pins Setup
  pinMode(msgReceivedLed, OUTPUT);
  pinMode(dangerLed, OUTPUT);

  // Ensure LEDs are off initially
  digitalWrite(msgReceivedLed, LOW);
  digitalWrite(dangerLed, LOW);

  Serial.println("Receiver ready.");
}

void loop() {
  // Check for new data from nRF24L01 asynchronously
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));
    receivedNewData = true; // Set the flag for new data

    // Light up the message received LED
    digitalWrite(msgReceivedLed, HIGH);

    Serial.print("##################Received Latitude: ");
    Serial.println(receivedData.latitude, 6);
    Serial.print("##################Received Longitude: ");
    Serial.println(receivedData.longitude, 6);
    Serial.print("##################Received Speed: ");
    Serial.print(receivedData.speed, 2);
    Serial.println(" m/s");

    // Calculate the distance and effective distance
    double distance = calculateDistance(localLatitude, localLongitude, receivedData.latitude, receivedData.longitude);

    // Check if danger condition is met
    if (distance < 50.0) { // Example danger threshold
      Serial.println("#######DANGER!!!!#######");
      digitalWrite(dangerLed, HIGH); // Light up the danger LED
    } else {
      digitalWrite(dangerLed, LOW); // Turn off danger LED
    }

    // Send a default acknowledgment back to the transmitter
    radio.stopListening(); // Switch to transmitter mode
    const char ack[] = "Ack: Received";
    bool success = radio.write(&ack, sizeof(ack)); // Send acknowledgment
    if (success) {
      Serial.println("Default Ack sent successfully.");
    } else {
      Serial.println("Failed to send Default Ack.");
    }

    radio.startListening(); // Switch back to receiver mode

    delay(100); // Keep the LED on briefly
    digitalWrite(msgReceivedLed, LOW); // Turn off message received LED
  }

  // Check for new GPS data asynchronously
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    if (gps.location.isUpdated()) {
      localLatitude = gps.location.lat();
      localLongitude = gps.location.lng();
      localSpeed = gps.speed.kmph() * 27.7778; // Convert to cm/s

      Serial.print("Local Latitude: ");
      Serial.println(localLatitude, 6);
      Serial.print("Local Longitude: ");
      Serial.println(localLongitude, 6);
      Serial.print("Local Speed: ");
      Serial.print(localSpeed, 2);
      Serial.println(" cm/s");
    }
  }

  // If new data has been received, calculate the distance
  if (receivedNewData) {
    // Reset the flag
    receivedNewData = false;
  }
}
