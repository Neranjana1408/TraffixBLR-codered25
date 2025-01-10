#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); 
const byte address[6] = "00001";


struct Location {
  double latitude;   
  double longitude;
};

void setup() {
  Serial.begin(9600);

  if (!radio.begin()) {
    Serial.println("nRF24L01 not detected! Check connections.");
    while (1); 
  }

  radio.openReadingPipe(1, address);
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setAutoAck(false); 
  radio.startListening();

  Serial.println("Receiver Ready");
}

void loop() {
  if (radio.available()) {
    Location receivedData;

    radio.read(&receivedData, sizeof(receivedData));

    Serial.println("Received GPS data:");
    Serial.print("Latitude: ");
    Serial.println(receivedData.latitude, 6); 
    Serial.print("Longitude: ");
    Serial.println(receivedData.longitude, 6); 
  }
}
