#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>


static const int RXPin = 3, TXPin = 4;
static const uint32_t GPSBaud = 9600; 
SoftwareSerial gpsSerial(RXPin, TXPin); 
TinyGPSPlus gps; 

RF24 radio(9, 10); 
const byte address[6] = "00001";


const int ledPin = 9; 

struct Location {
  double latitude;   
  double longitude;  
  double speed;     
};

void setup() {
  Serial.begin(9600);

  gpsSerial.begin(GPSBaud);

  pinMode(ledPin, OUTPUT);

  if (!radio.begin()) {
    Serial.println("nRF24L01 not detected! Check connections.");
    while (1); 
  }

  radio.openWritingPipe(address);
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setAutoAck(false); 
  radio.stopListening();

  Serial.println("Transmitter Ready");
}

void loop() {
  Location dataToSend;

  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());

    if (gps.location.isUpdated()) { 
      dataToSend.latitude = gps.location.lat();  
      dataToSend.longitude = gps.location.lng(); 
      dataToSend.speed = int(gps.speed.kmph() * 0.277778 * 100 );

      bool success = radio.write(&dataToSend, sizeof(dataToSend));

      if (success) {
        Serial.println("Message sent successfully.");
        Serial.print("Latitude: ");
        Serial.println(dataToSend.latitude, 6); 
        Serial.print("Longitude: ");
        Serial.println(dataToSend.longitude, 6); 
        Serial.print("Speed (cm/s): ");
        Serial.println(dataToSend.speed, 2); 

        digitalWrite(ledPin, HIGH); 
        delay(500);
        digitalWrite(ledPin, LOW);
      } else {
        Serial.println("Message failed to send.");
      }

      delay(10);

      radio.startListening();
      delay(10);

      if (radio.available()) {
        char ack[32] = ""; 
        radio.read(&ack, sizeof(ack)); 
        Serial.print("Received ACK: ");
        Serial.println(ack);
      } else {
        Serial.println("No ACK received.");
      }

      radio.stopListening(); 

      delay(1000); 
    }
  }
}
