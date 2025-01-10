#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Keypad.h>
#include <TimeLib.h> // For time management
#include <TinyGPS++.h> // For GPS module
#include <SoftwareSerial.h> // For software serial communication

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
SoftwareSerial gpsSerial(10, 11); // RX = 10, TX = 11 (connect GPS TX to pin 10, GPS RX to pin 11)

// Hardcoded vehicle and tower location
float vehicleLatitude = 37.7745;  // Initially within 500m
float vehicleLongitude = -122.4190;
float towerLatitude = 37.7750;  
float towerLongitude = -122.4195;

// Vehicle data
struct VehicleData {
  String vehicleDetails;
  String entryTime;
  String messagesSent;
  String exitTime;
};

VehicleData vehicle = {
  "Vehicle-001",  // Vehicle ID
  "",             // Entry time (initially empty)
  "",             // Messages sent (initially empty)
  ""              // Exit time (initially empty)
};

// Tracking variables
bool isVehicleInRadius = false;
unsigned long startTime = 0;

// Function declarations
bool isWithinRadius();
float calculateDistance(float lat1, float lon1, float lat2, float lon2);
void logEntry();
void logExit();
void handleKeyPress(char key);
String getCurrentTime();
void printCSV();
unsigned long getGPSTime();  // GPS time extraction function

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600); // Initialize GPS UART

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

  // Sync time with GPS
  setSyncProvider(getGPSTime); // Use custom function to provide GPS time

  // Check initial radius
  if (isWithinRadius()) {
    isVehicleInRadius = true;
    logEntry();
  }

  startTime = millis(); // Start time for simulation
}

void loop() {
  // Process GPS data
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Simulate movement after 1 minute
  if (millis() - startTime >= 60000) {
    vehicleLatitude = 37.7690;  // Move out of range
    vehicleLongitude = -122.4194;
  }

  if (isWithinRadius()) {
    if (!isVehicleInRadius) {
      isVehicleInRadius = true;
      logEntry();
    }

    // Handle keypad input
    char key = keypad.getKey();
    if (key) {
      handleKeyPress(key);
    }
  } else {
    if (isVehicleInRadius) {
      isVehicleInRadius = false;
      logExit();
    }
  }
}

// Function definitions
bool isWithinRadius() {
  float distance = calculateDistance(vehicleLatitude, vehicleLongitude, towerLatitude, towerLongitude);
  return distance <= 500;
}

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

void logEntry() {
  vehicle.entryTime = getCurrentTime();
  vehicle.messagesSent = "None"; // Reset messages
  vehicle.exitTime = "N/A"; // Reset exit time

  // Print entry log in CSV format
  printCSV();
}

void logExit() {
  vehicle.exitTime = getCurrentTime();

  // Print exit log in CSV format
  printCSV();
}

void handleKeyPress(char key) {
  String message;
  if (key == '4') message = "Ambulance";
  else if (key == '5') message = "Accident";
  else if (key == '6') message = "Traffic";
  else return;

  // Set the current message if not set or append to the existing one
  if (vehicle.messagesSent == "None" || vehicle.messagesSent == "") {
    vehicle.messagesSent = message;
  } else {
    vehicle.messagesSent = message; // Only keep the current message
  }

  // Print updated CSV with only the current message
  printCSV();

  // Display message on OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Message Sent:");
  display.setCursor(0, 10);
  display.print(message);
  display.display();
}

String getCurrentTime() {
  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", hour(), minute(), second());
  return String(buffer);
}

void printCSV() {
  Serial.print(vehicle.vehicleDetails);
  Serial.print(",");
  Serial.print(vehicle.entryTime);
  Serial.print(",");
  Serial.print(vehicle.messagesSent); // Print the current message only
  Serial.print(",");
  Serial.println(vehicle.exitTime);
}

// GPS time extraction function
unsigned long getGPSTime() {
  if (gps.time.isUpdated()) {
    return gps.time.value(); // Return the GPS time as an unsigned long value (seconds)
  }
  return 0; // If no valid GPS time is available, return 0
}
