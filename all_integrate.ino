#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Keypad.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C

// NRF24L01 setup
RF24 radio(9, 10); // CE, CSN pins
const byte address[6] = "00001";

// Hardcoded tower location
const float towerLatitude = 12.9718; // Example latitude
const float towerLongitude = 77.5947; // Example longitude

// Initialize OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Keypad setup
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {2, 3, 4, 5};   // Connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9};   // Connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// State tracking
bool isInRange = false;

// Function to calculate distance (Haversine formula approximation)
float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  return sqrt(pow(lat2 - lat1, 2) + pow(lon2 - lon1, 2)) * 111139; // Convert degrees to meters
}

// Function to track the car and log events
void trackCar(float carLatitude, float carLongitude) {
  float distance = calculateDistance(carLatitude, carLongitude, towerLatitude, towerLongitude);

  if (distance <= 500) {
    if (!isInRange) {
      isInRange = true;
      Serial.println("Hello Tower - Car entered 500m radius");
      displayMessage("Hello Tower");
      sendMessage("Hello Tower");
    }
  } else {
    if (isInRange) {
      isInRange = false;
      Serial.println("Bye Tower - Car exited 500m radius");
      displayMessage("Bye Tower");
      sendMessage("Bye Tower");
    }
  }
}

// Function to send messages to the tower
void sendMessage(const char* message) {
  if (isInRange) { // Only send messages when in range
    if (radio.write(&message, sizeof(message))) {
      Serial.println("Message sent: " + String(message));
    } else {
      Serial.println("Message failed.");
    }
  }
}

// Function to display messages on the OLED
void displayMessage(const char* message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(message);
  display.display();
}

void setup() {
  Serial.begin(9600);

  // Initialize NRF24L01
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW);
  radio.stopListening();

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Car System Ready!");
  display.display();
}

void loop() {
  // Simulated GPS coordinates (Replace with actual GPS module data)
  float carLatitude = 12.9716; // Example latitude
  float carLongitude = 77.5946; // Example longitude

  // Track car's location relative to the tower
  trackCar(carLatitude, carLongitude);

  // Handle keypad inputs (only when in range)
  if (isInRange) {
    char key = keypad.getKey();
    if (key) {
      switch (key) {
        case '4':
          Serial.println("Accident detected!");
          displayMessage("Accident Detected!");
          sendMessage("Accident Detected!");
          break;
        case '5':
          Serial.println("Call Ambulance!");
          displayMessage("Call Ambulance!");
          sendMessage("Call Ambulance!");
          break;
        case '6':
          Serial.println("Call Police!");
          displayMessage("Call Police!");
          sendMessage("Call Police!");
          break;
        default:
          Serial.println("Key pressed: " + String(key));
          displayMessage(("Key: " + String(key)).c_str());
          sendMessage(("Key: " + String(key)).c_str());
          break;
      }
    }
  }

  delay(1000); // Simulate periodic updates
}
