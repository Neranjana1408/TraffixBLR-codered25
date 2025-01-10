#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>


RF24 radio(9, 10);
const byte address[6] = "00001";

static const int RXPin = 3, TXPin = 4; 
static const uint32_t GPSBaud = 9600; 
SoftwareSerial gpsSerial(RXPin, TXPin); 
TinyGPSPlus gps; 

struct Location {
  float latitude;  
  float longitude;  
};

void setup() {
  Serial.begin(9600);

  if (!radio.begin()) {
    Serial.println("nRF24L01 not detected! Check connections.");
    while (1);
  }

  radio.openReadingPipe(0, address);
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(false);
  radio.startListening();


  gpsSerial.begin(GPSBaud);
  Serial.println("Receiver ready.");
}

void loop() {

  if (radio.available()) {
    Location receivedData;
    radio.read(&receivedData, sizeof(receivedData));

    Serial.print("Received Latitude: ");
    Serial.println(receivedData.latitude, 6);
    Serial.print("Received Longitude: ");
    Serial.println(receivedData.longitude, 6);
  }

  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    if (gps.location.isUpdated()) {
      float localLatitude = gps.location.lat();
      float localLongitude = gps.location.lng();

      Serial.print("Local Latitude: ");
      Serial.println(localLatitude, 6);
      Serial.print("Local Longitude: ");
      Serial.println(localLongitude, 6);
    }
  }
}
