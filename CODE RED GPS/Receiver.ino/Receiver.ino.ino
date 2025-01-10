#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

RF24 radio(9, 10); 
const byte address[6] = "00001";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

struct Location {
  float latitude;
  float longitude;
};

void setup() {
  Serial.begin(9600);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(false); 
  radio.startListening();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  if (radio.available()) {
    Location receivedData;
    radio.read(&receivedData, sizeof(receivedData));
    
    Serial.print("Received Latitude: ");
    Serial.println(receivedData.latitude);
    Serial.print("Received Longitude: ");
    Serial.println(receivedData.longitude);


    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Lat: ");
    display.println(receivedData.latitude);
    display.setCursor(0, 16);
    display.print("Long: ");
    display.println(receivedData.longitude);
    display.display();
  }
}
