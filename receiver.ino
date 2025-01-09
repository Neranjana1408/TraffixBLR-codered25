#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Keypad.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// OLED Display Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// nRF24L01 Setup
RF24 radio(9, 10); // CE, CSN pins
const byte address[6] = "00001";

// GPS Setup
static const int RXPin = 3, TXPin = 4;
static const uint32_t GPSBaud = 9600;
SoftwareSerial gpsSerial(RXPin, TXPin);
TinyGPSPlus gps;

// Keypad Setup
const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, 9};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Tower Coordinates
const float towerLatitude = 12.9718;
const float towerLongitude = 77.5947;

// State Tracking
bool isInRange = false;
char lastExitTime[10] = "--"; // Placeholder for exit time
char vehicleDetails[] = "Car123"; // Example vehicle details

// Struct for Receiving GPS Data via nRF24L01
struct Location {
    float latitude;
    float longitude;
};

void setup() {
    Serial.begin(9600);

    // Initialize nRF24L01
    if (!radio.begin()) {
        Serial.println("nRF24L01 not detected! Check connections.");
        while (1);
    }
    radio.openReadingPipe(0, address);
    radio.setChannel(108);
    radio.setPALevel(RF24_PA_LOW);
    radio.setAutoAck(false);
    radio.startListening();

    // Initialize OLED Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        for (;;);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("System Ready!");
    display.display();

    // Initialize GPS
    gpsSerial.begin(GPSBaud);
}

void logData(const char* message, float latitude, float longitude, const char* exitTime, float speed) {
    char timestamp[20];
    if (gps.time.isValid()) {
        snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    } else {
        strcpy(timestamp, "Invalid Time");
    }
    Serial.print(vehicleDetails);
    Serial.print(",");
    Serial.print(latitude);
    Serial.print(",");
    Serial.print(longitude);
    Serial.print(",");
    Serial.print(timestamp);
    Serial.print(",");
    Serial.print(exitTime);
    Serial.print(",");
    Serial.print(speed);
    Serial.print(",");
    Serial.println(message);
}

void displayMessage(const char* message) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(message);
    display.display();
}

void sendMessage(const char* message) {
    if (radio.write(&message, sizeof(message))) {
        Serial.println(String("Message sent: ") + message);
    } else {
        Serial.println("Message failed.");
    }
}

float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
    return sqrt(pow(lat2 - lat1, 2) + pow(lon2 - lon1, 2)) * 111139; // Approximation
}

void trackCar(float latitude, float longitude) {
    float distance = calculateDistance(latitude, longitude, towerLatitude, towerLongitude);
    float speed = gps.speed.kmph();

    if (distance <= 500) {
        if (!isInRange) {
            isInRange = true;
            strcpy(lastExitTime, "--");
            logData("Hi Tower", latitude, longitude, lastExitTime, speed);
            displayMessage("Hello Tower");
            sendMessage("Hi Tower");
        }
    } else {
        if (isInRange) {
            isInRange = false;
            if (gps.time.isValid()) {
                snprintf(lastExitTime, sizeof(lastExitTime), "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
            } else {
                strcpy(lastExitTime, "Invalid Time");
            }
            logData("Bye Tower", latitude, longitude, lastExitTime, speed);
            displayMessage("Bye Tower");
            sendMessage("Bye Tower");
        }
    }
}

void loop() {
    // Handle incoming GPS data via nRF24L01
    if (radio.available()) {
        Location receivedData;
        radio.read(&receivedData, sizeof(receivedData));
        Serial.print("Received Latitude: ");
        Serial.println(receivedData.latitude, 6);
        Serial.print("Received Longitude: ");
        Serial.println(receivedData.longitude, 6);
    }

    // Handle GPS data from the local GPS module
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }

    if (gps.location.isValid()) {
        float latitude = gps.location.lat();
        float longitude = gps.location.lng();
        trackCar(latitude, longitude);
    }

    // Handle keypad inputs
    if (isInRange) {
        char key = keypad.getKey();
        if (key) {
            char message[30];
            switch (key) {
                case '4':
                    strcpy(message, "Accident Detected!");
                    break;
                case '5':
                    strcpy(message, "Call Ambulance!");
                    break;
                case '6':
                    strcpy(message, "Call Police!");
                    break;
                default:
                    snprintf(message, sizeof(message), "Key: %c", key);
                    break;
            }
            logData(message, gps.location.lat(), gps.location.lng(), lastExitTime, gps.speed.kmph());
            displayMessage(message);
            sendMessage(message);
        }
    }

    delay(1000); // Delay to avoid overloading
}
