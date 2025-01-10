tr#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Keypad.h>
#include <TinyGPS++.h> 
#include <SoftwareSerial.h> 
#include <TimeLib.h> 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


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


TinyGPSPlus gps;
SoftwareSerial gpsSerial(10, 11); 
float towerLatitude = 37.7749; 
float towerLongitude = -122.4194; 


struct VehicleData {
  String vehicleDetails;
  String entryTime;
  String messagesSent;
  String exitTime;
};

VehicleData vehicle = {
  "Vehicle-001",  
  "",             
  "",             
  ""              
};


bool isVehicleInRadius = false;
unsigned long lastKeyPressTime = 0;
char lastKeys[3] = {0}; 
int keyIndex = 0;

void setup() {
  Serial.begin(9600);
  gpsSerial.begin(9600); 
  
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

 
  Serial.println("Vehicle Details,Entry Time,Messages Sent,Exit Time");
}

void loop() {
  
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

    
      char key = keypad.getKey();
      if (key) {
        unsigned long currentTime = millis();

       
        if (currentTime - lastKeyPressTime <= 3000) {
          lastKeys[keyIndex] = key;
          keyIndex = (keyIndex + 1) % 3; 
        } else {
          keyIndex = 0; 
          lastKeys[keyIndex] = key;
        }
        lastKeyPressTime = currentTime;

      
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


float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371000; 
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
  vehicle.exitTime = "N/A"; 
  vehicle.messagesSent = "None"; 

  printCSV();
}

void logExit() {
  vehicle.exitTime = getCurrentTime();


  printCSV();
}


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

  
  printCSV();
}


void printCSV() {
  Serial.print(vehicle.vehicleDetails);
  Serial.print(",");
  Serial.print(vehicle.entryTime);
  Serial.print(",");
  Serial.print(vehicle.messagesSent);
  Serial.print(",");
  Serial.println(vehicle.exitTime);
}


String getCurrentTime() {
  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", hour(), minute(), second());
  return String(buffer);
}


void displayPriorityMessages() {
  display.clearDisplay();
  bool priorityDisplayed[3] = {false, false, false}; 

  for (int i = 0; i < 3; i++) {
    char key = lastKeys[i];
    if (key == '4' && !priorityDisplayed[0]) { 
      display.setCursor(0, i * 10);
      display.print("Priority 1: Ambulance");
      priorityDisplayed[0] = true;
    } else if (key == '5' && !priorityDisplayed[1]) { 
      display.setCursor(0, i * 10);
      display.print("Priority 2: Accident");
      priorityDisplayed[1] = true;
    } else if (key == '6' && !priorityDisplayed[2]) { 
      display.setCursor(0, i * 10);
      display.print("Priority 3: Traffic");
      priorityDisplayed[2] = true;
    }
  }
  display.display();
}
