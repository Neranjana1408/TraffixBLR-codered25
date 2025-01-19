/************************************************** 
   Example: Receive and Display nRF24L01 Data on OLED
   Hardware: 
     - Arduino UNO (or similar)
     - nRF24L01 (pins as per your existing wiring)
     - SSD1306 OLED (I2C) 
     - GPS module (optional if you want local lat/lng)
***************************************************/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// ----------- OLED Libraries -----------
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Change these to match your actual display size if it’s 120×32
// but often you might just do 128×32:
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// Create the display object (I2C)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- nRF24 Setup ----------
RF24 radio(9, 10);           // CE = 9, CSN = 10
const byte address[6] = "00001";

// ---------- GPS Setup ------------
static const int RXPin = 3, TXPin = 4;
static const uint32_t GPSBaud = 9600;
SoftwareSerial gpsSerial(RXPin, TXPin);
TinyGPSPlus gps;

struct Location {
  float latitude;  
  float longitude;  
};

void setup() {
  // ---------- Serial Monitor -----------
  Serial.begin(9600);
  
  // ---------- nRF24 Init -----------
  if (!radio.begin()) {
    Serial.println("nRF24L01 not detected! Check connections.");
    while (1);
  }
  radio.openReadingPipe(0, address);
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(false);
  radio.startListening();
  Serial.println("nRF24 Receiver ready.");

  // ---------- GPS Init -----------
  gpsSerial.begin(GPSBaud);
  Serial.println("Please wait while GPS is getting lock...");

  // ---------- OLED Init -----------
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C is common I2C address
    Serial.println("OLED not found. Check wiring/address.");
    while(1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);       // Normal 1:1 pixel scale
  display.setTextColor(WHITE);  // Draw white text
  display.setCursor(0, 0);
  display.println("Receiver Init...");
  display.display();            // Update the screen with the initial text
  delay(1000);
}

void loop() {
  // Check if we have data from the nRF24
  if (radio.available()) {
    Location receivedData;
    radio.read(&receivedData, sizeof(receivedData));

    // Print to Serial
    Serial.print("Received Latitude: ");
    Serial.println(receivedData.latitude, 6);
    Serial.print("Received Longitude: ");
    Serial.println(receivedData.longitude, 6);

    // Display on OLED
    display.clearDisplay();      // Clear buffer
    display.setCursor(0, 0);
    // display.println("Received Data:");
    
    display.print("- - - - - \n");
    display.print("Lat: ");
    display.println(receivedData.latitude, 6);
    display.print("- - - - - \n");
    display.print("Lng: ");
    display.println(receivedData.longitude, 6);
    display.display();           // Update OLED
  }

  // If you want to read your local GPS on the same board:
  // (Uncomment if you want to see local lat/lng or display it too)
  /*
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    if (gps.location.isUpdated()) {
      float localLatitude = gps.location.lat();
      float localLongitude = gps.location.lng();

      Serial.print("Local Latitude: ");
      Serial.println(localLatitude, 6);
      Serial.print("Local Longitude: ");
      Serial.println(localLongitude, 6);

      // If you also want to display local GPS on the OLED, do:
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Received Data:");
      display.print("Lat: ");
      display.println(localLatitude, 6);
      display.print("Lng: ");
      display.println(localLongitude, 6);
      display.display();
    }
  }
  */
}
