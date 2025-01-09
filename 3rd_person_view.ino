#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// nRF24L01 Setup
RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

// Structure to receive GPS data
struct Location {
  double latitude;   // Latitude received from the transmitter
  double longitude;  // Longitude received from the transmitter
};

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // nRF24L01 Initialization
  if (!radio.begin()) {
    Serial.println("nRF24L01 not detected! Check connections.");
    while (1); // Halt
  }

  radio.openReadingPipe(1, address);
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setAutoAck(false); // Disable Auto-ACK
  radio.startListening();

  Serial.println("Receiver Ready");
}

void loop() {
  // Check if there is data available from the transmitter
  if (radio.available()) {
    Location receivedData;

    // Read the received GPS data
    radio.read(&receivedData, sizeof(receivedData));

    // Print the received data to the Serial Monitor
    Serial.println("Received GPS data:");
    Serial.print("Latitude: ");
    Serial.println(receivedData.latitude, 6); // Print latitude with 6 decimal places
    Serial.print("Longitude: ");
    Serial.println(receivedData.longitude, 6); // Print longitude with 6 decimal places
  }
}
