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

// LED Setup
const int ledPin = 9; // LED pin on pin 9

// Structure to send GPS data
struct Location {
  double latitude;   // Latitude with high precision
  double longitude;  // Longitude with high precision
  double speed;      // Speed in cm/s
};
String hi ="connection established";
void setup() {
  // Serial Monitor
  Serial.begin(9600);

  // GPS Initialization
  gpsSerial.begin(GPSBaud);

  // LED Initialization
  pinMode(ledPin, OUTPUT); // Set LED pin as output

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
  radio.write(&hi, sizeof(hi));
  Serial.println("Transmitter Ready");
}

void loop() {
  Location dataToSend;
  dataToSend.latitude = 9.99999;
  dataToSend.longitude = 9.99999 ;
  // Wait for valid GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    
    if (gps.location.isUpdated()) { // Check for updated location data
      dataToSend.latitude = gps.location.lat();   // Get latitude
      dataToSend.longitude = gps.location.lng(); // Get longitude
      dataToSend.speed = int(gps.speed.kmph() * 0.277778 * 100 ); // Convert speed to cm/s

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

        // Turn on LED to indicate successful transmission
        digitalWrite(ledPin, HIGH); // LED on
        delay(500); // LED stays on for 0.5 seconds
        digitalWrite(ledPin, LOW);  // LED off
      } else {
        Serial.println("Message failed to send.");
      }

      // Wait briefly to allow the receiver to send acknowledgment
      delay(10);

      // Check for acknowledgment
      radio.startListening(); // Switch to listening mode for acknowledgment
      delay(10);

      if (radio.available()) {
        char ack[32] = ""; // Buffer for acknowledgment message
        radio.read(&ack, sizeof(ack)); // Read the acknowledgment message
        Serial.print("Received ACK: ");
        Serial.println(ack);
        Serial.println("############ack sent"); // Print the desired message
      } else {
        Serial.println("No ACK received.");
      }

      radio.stopListening(); // Switch back to transmitting mode

      delay(1000); // Wait 1 second before sending again
    }
  }
}
