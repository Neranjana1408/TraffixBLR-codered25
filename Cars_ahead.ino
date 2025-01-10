#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <math.h> // Required for mathematical functions
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// nRF24L01 Setup
RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";     // Address for incoming
const byte ackAddress[6] = "00002";  // Address for outgoing acknowledgment

// OLED setup for 128x32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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

// Function to calculate the magnitude of the least possible distance with ±5m error margin
double calculateEffectiveDistance(double actualDistance) {
  double lowerBound = actualDistance - 10.0; // Lower bound of the range
  double upperBound = actualDistance + 10.0; // Upper bound of the range

  // Calculate magnitudes of the bounds
  double magnitudeLower = abs(lowerBound);
  double magnitudeUpper = abs(upperBound);

  // Return the smallest magnitude
  return (magnitudeLower < magnitudeUpper) ? magnitudeLower : magnitudeUpper;
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

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // GPS Initialization
  gpsSerial.begin(GPSBaud);
  Serial.println("Receiver ready.");
}

void loop() {
  // Check for new data from nRF24L01 asynchronously
  if (radio.available()) {
    radio.read(&receivedData, sizeof(receivedData));
    receivedNewData = true; // Set the flag for new data

    Serial.print("##################Received Latitude: ");
    Serial.println(receivedData.latitude, 6);
    Serial.print("##################Received Longitude: ");
    Serial.println(receivedData.longitude, 6);
    Serial.print("##################Received Speed: ");
    Serial.print(receivedData.speed, 2);
    Serial.println(" m/s");

    // Display received data on OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Received:");
    display.setCursor(0, 16);
    display.print("Lat: ");
    display.print(receivedData.latitude, 6);
    display.setCursor(0, 24);
    display.print("Lon: ");
    display.print(receivedData.longitude, 6);
    display.display();

    // Send acknowledgment back to transmitter
    radio.stopListening(); // Switch to transmitter mode
    const char ack[] = "Data Received";
    bool success = radio.write(&ack, sizeof(ack)); // Send acknowledgment
    if (success) {
      Serial.println("Ack sent successfully.");
    } else {
      Serial.println("Failed to send Ack.");
    }
    radio.startListening(); // Switch back to receiver mode
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
    if (localLatitude != 0.0 && localLongitude != 0.0) {
      // Calculate distance between local and received GPS coordinates
      double distance = calculateDistance(localLatitude, localLongitude, receivedData.latitude, receivedData.longitude);

      // Calculate effective distance with error margin
      double effectiveDistance = calculateEffectiveDistance(distance);

      Serial.print("Distance to Received Location: ");
      Serial.print(effectiveDistance, 2); // Print effective distance with 2 decimal places
      Serial.println(" meters");

      // Calculate the braking distance
      double brakingDistance = pow(localSpeed , 2) / (2 * 0.6 * 9.8); // Speed converted to m/s for calculation
      Serial.print("Braking Distance: ");
      Serial.println(brakingDistance, 2);

      if (effectiveDistance < brakingDistance) {
        Serial.println("#######DANGER!!!!#######");
      }
    } else {
      Serial.println("Waiting for local GPS fix...");
    }

    // Reset the flag
    receivedNewData = false;
  }
}
