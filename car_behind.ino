#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// GPS Setup
static const int RXPin = 3, TXPin = 4; // Connect GPS TX → Arduino Pin 3, GPS RX → Arduino Pin 4
static const uint32_t GPSBaud = 9600; // GPS Baud rate
SoftwareSerial gpsSerial(RXPin, TXPin); // GPS serial communication
TinyGPSPlus gps; // GPS object

// nRF24L01 Setup
RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

// Structure to send GPS data
struct Location {
  double latitude;   // Latitude with high precision
  double longitude;  // Longitude with high precision
  double speed;      // Speed in cm/s
};

void setup() {
  // Serial Monitor
  Serial.begin(9600);

  // GPS Initialization
  gpsSerial.begin(GPSBaud);

  // nRF24L01 Initialization
  if (!radio.begin()) {
    Serial.println("nRF24L01 not detected! Check connections.");
    while (1); // Halt
  }

  radio.openWritingPipe(address);
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setAutoAck(false); // Disable Auto-ACK
  radio.stopListening();

  Serial.println("Transmitter Ready");
}

void loop() {
  Location dataToSend;

  // Wait for valid GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());

    if (gps.location.isUpdated()) { // Check for updated location data
      dataToSend.latitude = gps.location.lat();   // Get latitude
      dataToSend.longitude = gps.location.lng(); // Get longitude
      dataToSend.speed = int(gps.speed.kmph() * 0.277778*0.6 ); // Convert speed to cm/s

      // Transmit GPS coordinates
      bool success = radio.write(&dataToSend, sizeof(dataToSend));

      if (success) {
        Serial.println("Message sent successfully.");
        Serial.print("Latitude: ");
        Serial.println(dataToSend.latitude, 6); // Print latitude with 6 decimal places
        Serial.print("Longitude: ");
        Serial.println(dataToSend.longitude, 6); // Print longitude with 6 decimal places
        Serial.print("Speed (cm/s): ");
        Serial.println(dataToSend.speed, 2); // Print speed with 2 decimal places
      } else {
        Serial.println("Message failed to send.");
      }
      delay(1000); // Wait 1 second before sending again
    }
  }
}
