#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

RF24 radio(9, 10); // CE, CSN
const byte address[6] = "00001";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(9600);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setChannel(108);
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(false); // Disable Auto-ACK
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
    char text[32] = "";
    radio.read(&text, sizeof(text));

    Serial.print("Received: ");
    Serial.println(text);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Received:");
    display.setCursor(0, 16);
    display.println(text);
    display.display();
  }
}
