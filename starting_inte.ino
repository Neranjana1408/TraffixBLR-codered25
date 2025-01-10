tr#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Keypad.h>
#include <TinyGPS++.h> // For GPS handling
#include <SoftwareSerial.h> // For GPS communication
#include <TimeLib.h> // For managing time

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// GPS setup
TinyGPSPlus gps;
SoftwareSerial gpsSerial(10, 11); // RX, TX for GPS module

// Tower GPS location (hardcoded)
float towerLatitude = 37.7749;  // Example latitude
float towerLongitude = -122.4194; // Example longitude

// Vehicle data structure
struct VehicleData {
  String vehicleDetails;
  String entryTime;
  String messagesSent;
  String exitTime;
};

VehicleData vehicle = {
  "Vehicle-001",  // Example vehicle ID
  "",             // Entry time (initially empty)
  "",             // Messages sent (initially empty)
  ""              // Exit time (initially empty)
};

// Tracking variables
bool isVehicleInRadius = false;
unsigned long lastKeyPressTime = 0;
char lastKeys[3] = {0}; // Stores up to 3 keys pressed
int keyIndex = 0;

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600); // GPS module baud rate
  
  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("System Ready");
  display.display();
  delay(2000);
  display.clearDisplay();

  // Print CSV Header
  Serial.println("Vehicle Details,Entry Time,Messages Sent,Exit Time");
}

void loop() {
  // Update GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated()) {
    float vehicleLatitude = gps.location.lat();
    float vehicleLongitude = gps.location.lng();
    float distance = calculateDistance(vehicleLatitude, vehicleLongitude, towerLatitude, towerLongitude);

    if (distance <= 500) {
      if (!isVehicleInRadius) {
        isVehicleInRadius = true;
        logEntry();
      }

      // Handle keypad input
      char key = keypad.getKey();
      if (key) {
        unsigned long currentTime = millis();

        // Handle multi-key logic
        if (currentTime - lastKeyPressTime <= 3000) {
          lastKeys[keyIndex] = key;
          keyIndex = (keyIndex + 1) % 3; // Rotate keys
        } else {
          keyIndex = 0; // Reset on timeout
          lastKeys[keyIndex] = key;
        }
        lastKeyPressTime = currentTime;

        // Update message log
        updateMessageLog(key);
        displayPriorityMessages();
      }
    } else {
      if (isVehicleInRadius) {
        isVehicleInRadius = false;
        logExit();
      }
    }
  }
}

// Function to calculate distance between two GPS coordinates in meters
float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371000; // Earth radius in meters
  float phi1 = lat1 * (M_PI / 180.0);
  float phi2 = lat2 * (M_PI / 180.0);
  float deltaPhi = (lat2 - lat1) * (M_PI / 180.0);
  float deltaLambda = (lon2 - lon1) * (M_PI / 180.0);
  float a = sin(deltaPhi / 2) * sin(deltaPhi / 2) +
            cos(phi1) * cos(phi2) *
            sin(deltaLambda / 2) * sin(deltaLambda / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

// Log entry event
void logEntry() {
  vehicle.entryTime = getCurrentTime();
  vehicle.exitTime = "N/A"; // Reset exit time
  vehicle.messagesSent = "None"; // Reset messages

  // Print as CSV
  printCSV();
}

// Log exit event
void logExit() {
  vehicle.exitTime = getCurrentTime();

  // Print as CSV
  printCSV();
}

// Update message log
void updateMessageLog(char key) {
  String message;
  if (key == '4') message = "Ambulance";
  else if (key == '5') message = "Accident";
  else if (key == '6') message = "Traffic";
  else return;

  if (vehicle.messagesSent == "None") {
    vehicle.messagesSent = message;
  } else {
    vehicle.messagesSent += "; " + message;
  }

  // Print as CSV
  printCSV();
}

// Print data as CSV
void printCSV() {
  Serial.print(vehicle.vehicleDetails);
  Serial.print(",");
  Serial.print(vehicle.entryTime);
  Serial.print(",");
  Serial.print(vehicle.messagesSent);
  Serial.print(",");
  Serial.println(vehicle.exitTime);
}

// Get current time as a string
String getCurrentTime() {
  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", hour(), minute(), second());
  return String(buffer);
}

// Display messages based on priority
void displayPriorityMessages() {
  display.clearDisplay();
  bool priorityDisplayed[3] = {false, false, false}; // Track displayed priorities

  for (int i = 0; i < 3; i++) {
    char key = lastKeys[i];
    if (key == '4' && !priorityDisplayed[0]) { // Priority 1: Ambulance
      display.setCursor(0, i * 10);
      display.print("Priority 1: Ambulance");
      priorityDisplayed[0] = true;
    } else if (key == '5' && !priorityDisplayed[1]) { // Priority 2: Accident
      display.setCursor(0, i * 10);
      display.print("Priority 2: Accident");
      priorityDisplayed[1] = true;
    } else if (key == '6' && !priorityDisplayed[2]) { // Priority 3: Traffic
      display.setCursor(0, i * 10);
      display.print("Priority 3: Traffic");
      priorityDisplayed[2] = true;
    }
  }
  display.display();
}
