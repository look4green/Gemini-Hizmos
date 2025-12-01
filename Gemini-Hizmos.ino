/*
 * ------------------------------------------------
 * |       MULTI TOOL DEVICE: PENTESTING SYSTEM     |
 * ------------------------------------------------
 * | Target: ESP32-S3-N16R8                         |
 * | Display: 1.3-inch OLED (U8G2)                  |
 * | Control: 5 Buttons                             |
 * | Core Features: 10-point menu system            |
 * ------------------------------------------------
 */

#include "animations.h"
#include "mainmenu.h"
#include "dolphinreactions.h"
#include <stdint.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include <Update.h>
#include <vector>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <BleMouse.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <IRremote.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
// #include <TinyGPS++.h>       // ⚠️ Add this library for GPS
// #include <cc1101.h>          // ⚠️ Add this library for Sub-GHz

// ESP32 System Headers (Good for Status Checks)
#include <esp_wifi.h>
#include "esp_heap_caps.h"
#include "esp_spi_flash.h"
#include "esp_chip_info.h"
#include "esp_system.h"


// --- 1. GLOBALS & STRUCTS ---

// HID & BLE
USBHIDKeyboard Keyboard;
BleMouse mouse_ble("hizmos", "hizmos", 100);

// BLE Scanner State
struct blescanner_Device {
  String name;
  String address;
  int rssi;
  String manufacturer;
  String deviceType;
};

std::vector<blescanner_Device> blescanner_devices;
BLEScan* blescanner_pBLEScan;

// UI/System State
int mainMenuIndex = 1; // Current selected item (1-10)
const int MAX_MENU_INDEX = 10; 
const int MIN_MENU_INDEX = 1;
bool autoMode = true;
unsigned long lastImageChangeTime = 0;
unsigned long lastButtonPressTime = 0;
const unsigned long autoModeTimeout = 120000; // 2 minutes to switch back to auto

int autoImageIndex = 0;
const int totalAutoImages = 406;
const int totalManualImages = 11; // If this is used for manual image scrolling


// --- 2. PIN DEFINITIONS ---

// I2C (OLED & NFC)
#define I2C_SDA 8
#define I2C_SCL 9

// Buttons (Input Pullup)
#define BTN_UP      4
#define BTN_DOWN    5
#define BTN_SELECT  6
#define BTN_BACK    7
#define BTN_LEFT    1
#define BTN_RIGHT   2

// IR (Sender/Receiver)
#define irsenderpin  41
#define irrecivepin  40

// NeoPixel LED
#define LED_PIN     48
#define LED_COUNT   1

// NFC (PN532)
#define PN532_IRQ   22
#define PN532_RESET 21

// FSPI Bus (nRF Modules)
#define NRF_SCK    18
#define NRF_MISO   16
#define NRF_MOSI   17

// nRF24 Module Pins (Shared FSPI)
#define CE1_PIN    10
#define CSN1_PIN   11
#define CE2_PIN    12
#define CSN2_PIN   13
#define CE3_PIN    20 // New pin for 3rd nRF
#define CSN3_PIN   19 // New pin for 3rd nRF

// HSPI Bus (SD Card)
#define SD_SCK     14
#define SD_MISO    39
#define SD_MOSI    38
#define SD_CS      37

// CC1101 (Sub-GHz) Pins (Shares FSPI or secondary SPI)
#define CC1101_CS_PIN    33 // Dedicated Chip Select
#define CC1101_GDO0_PIN  34 // Dedicated Interrupt pin

// GPS Pins (Using UART2)
#define GPS_TX_PIN 15 // Connects to GPS RX pin
#define GPS_RX_PIN 42 // Connects to GPS TX pin (Data out)

// GPIO Testing Pins
#define ANALOG_PIN 3
#define WAVE_OUT_PIN 48 // WARNING: Shared with LED_PIN, use caution!


// --- 3. HARDWARE OBJECTS ---

// Display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// NeoPixel
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// NFC/RFID
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

// IR
IRrecv irrecv(irrecivepin);
IRsend irsend(irsenderpin);
decode_results results;

// SPI Buses
SPIClass RADIO_SPI(FSPI);
SPIClass SD_SPI(HSPI);

// RF24 Modules
RF24 radio1(CE1_PIN, CSN1_PIN);
RF24 radio2(CE2_PIN, CSN2_PIN);
RF24 radio3(CE3_PIN, CSN3_PIN); // Added third module

// GPS Module (Requires HardwareSerial instance)
// HardwareSerial GPSSerial(2); // Example using UART2
// TinyGPSPlus gps;


// --- 4. DEVICE CONTROL/HELPER FUNCTIONS ---

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void deactivateAllModules() {
  // Deactivate all Chip Selects
  digitalWrite(SD_CS, HIGH);
  digitalWrite(CSN1_PIN, HIGH);
  digitalWrite(CSN2_PIN, HIGH);
  digitalWrite(CSN3_PIN, HIGH);
  digitalWrite(CC1101_CS_PIN, HIGH); 
  
  // Disable Chip Enables (for nRF)
  digitalWrite(CE1_PIN, LOW);
  digitalWrite(CE2_PIN, LOW);
  digitalWrite(CE3_PIN, LOW);
}

// Function definitions for UI/Animation
void displayImage(const uint8_t* image) {
  u8g2.clearBuffer();
  u8g2.drawXBMP(0, 0, 128, 64, image);
  u8g2.sendBuffer();
}

void displaymainanim(const uint8_t* image, int batteryPercent, bool sdOK) {
  // ... (Removed large image array definitions for cleanup)
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);  // White
  u8g2.drawXBMP(0, 0, 128, 64, image);
  u8g2.setDrawColor(2);  // XOR mode for overlay
  // Add battery/SD card drawing logic here if desired
  u8g2.sendBuffer();
}

// --- 5. MENU DRAWING & LOGIC FUNCTIONS ---

const char* menuItems[] = {
    "1. WIFI ATTACKS", "2. BLE ATTACKS", "3. BAD USB", "4. NFC",
    "5. INFRARED", "6. SUB-GHZ", "7. 2.4GHZ RF", "8. APPS", 
    "9. SETTINGS", "10. FILES"
};

void drawMainMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 10);
    u8g2.print("--- MULTI TOOL ---");

    // Implement scrolling to draw visible items
    int startItem = max(MIN_MENU_INDEX, mainMenuIndex - 2); 
    int endItem = min(MAX_MENU_INDEX, startItem + 4);

    for (int i = startItem; i <= endItem; i++) {
        int displayY = 20 + (i - startItem) * 8;
        if (i == mainMenuIndex) {
            u8g2.drawRBox(0, displayY - 8, 128, 9, 0); // Highlight
            u8g2.setDrawColor(0); 
            u8g2.setCursor(3, displayY);
            u8g2.print(">");
            u8g2.print(menuItems[i - 1]);
            u8g2.setDrawColor(1); // Reset
        } else {
            u8g2.setCursor(3, displayY);
            u8g2.print(menuItems[i - 1]);
        }
    }
    u8g2.sendBuffer();
}

void runLoop(void (*func)()) {
    // This function runs a submenu loop until BTN_BACK is pressed
    // (Existing runLoop logic retained)
    unsigned long lastRun = millis();
    while (true) {
      if (millis() - lastRun > 130) {
        func();
        lastRun = millis();
      }
      if (digitalRead(BTN_BACK) == LOW) {
        while (digitalRead(BTN_BACK) == LOW);
        break;
      }
    }
}

// Placeholder Submenu Functions
void runWifiMenu() { 
  u8g2.setCursor(0, 30); u8g2.print("WiFi Menu Active"); 
  u8g2.sendBuffer();
}
void runBleMenu() { 
  u8g2.setCursor(0, 30); u8g2.print("BLE Menu Active"); 
  u8g2.sendBuffer();
}
// ... Add more submenu placeholders (runBadUsbMenu, runNfcMenu, etc.)

void handleButtonPress() {
    // Handle UP/DOWN Navigation
    if (digitalRead(BTN_UP) == LOW) {
        while (digitalRead(BTN_UP) == LOW); 
        mainMenuIndex = max(MIN_MENU_INDEX, mainMenuIndex - 1);
        lastButtonPressTime = millis();
    }
    if (digitalRead(BTN_DOWN) == LOW) {
        while (digitalRead(BTN_DOWN) == LOW);
        mainMenuIndex = min(MAX_MENU_INDEX, mainMenuIndex + 1);
        lastButtonPressTime = millis();
    }
    if (digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
        // Can be used for quick actions or horizontal movement within a sub-menu
        lastButtonPressTime = millis();
    }

    // Handle SELECT Button Action
    if (digitalRead(BTN_SELECT) == LOW) {
        while (digitalRead(BTN_SELECT) == LOW);
        
        // Execute the function based on the selected menu item
        switch (mainMenuIndex) {
            case 1: runLoop(runWifiMenu); break;
            case 2: runLoop(runBleMenu); break;
            // case 3: runLoop(runBadUsbMenu); break;
            // ... add all 10 cases ...
            default: break;
        }
    }
}


void handlemainmenu() {
    handleButtonPress();
    drawMainMenu();
}

// --- 6. SETUP & LOOP ---

void setup() {
  Serial.begin(9600);

  // 6.1. Pin Modes
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(WAVE_OUT_PIN, OUTPUT);
  pinMode(CC1101_CS_PIN, OUTPUT);
  pinMode(CC1101_GDO0_PIN, INPUT); // GDO pin is usually an input/IRQ source
  
  // Set all CS pins to output and high initially
  pinMode(SD_CS, OUTPUT);
  pinMode(CSN1_PIN, OUTPUT);
  pinMode(CSN2_PIN, OUTPUT);
  pinMode(CSN3_PIN, OUTPUT); // 3rd nRF
  pinMode(CE1_PIN, OUTPUT);
  pinMode(CE2_PIN, OUTPUT);
  pinMode(CE3_PIN, OUTPUT); // 3rd nRF

  deactivateAllModules();

  // 6.2. I2C Initialization (OLED & NFC)
  Wire.begin(I2C_SDA, I2C_SCL); // Initialize I2C on custom pins
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  nfc.begin();
  nfc.SAMConfig();

  // 6.3. SPI Initialization (RF & SD)
  RADIO_SPI.begin(NRF_SCK, NRF_MISO, NRF_MOSI); // FSPI for radios
  SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI);     // HSPI for SD
  // SD.begin(SD_CS, SD_SPI);                 // Needs to be done here or in checksysdevices

  // 6.4. RF and IR Initialization
  irrecv.begin(irrecivepin);
  irsend.begin(irsenderpin);
  // radio1.begin(&RADIO_SPI); 
  // radio2.begin(&RADIO_SPI); 
  // radio3.begin(&RADIO_SPI); // Initialize 3rd nRF
  // cc1101.init(CC1101_CS_PIN, CC1101_GDO0_PIN); // CC1101 initialization (placeholder)

  // 6.5. Other Modules (LED, USB, BLE, GPS)
  pixel.begin();
  pixel.setBrightness(80);
  pixel.show();
  USB.begin();
  Keyboard.begin();
  // GPSSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); // Initialize GPS UART (placeholder)
  
  // 6.6. Initial System Status Display
  drawstartinfo();
  delay(1000);
  u8g2.clearBuffer();
  displayImage(config2);
  u8g2.sendBuffer();
  delay(1000);
  checksysdevices(); // This should check if SD, nRF, NFC, etc., initialized OK
  delay(1000);
}

void loop() {
  // Check for button press status to switch modes
  handleButtons(); 
  autoModeCheck();
  setColor(0, 0, 0); // Keep LED off unless used for notification

  if (autoMode) {
    // Run the animation loop
    if (millis() - lastImageChangeTime > 300) {
      autoImageIndex = (autoImageIndex + 1) % totalAutoImages;
      lastImageChangeTime = millis();
    }
    displaymainanim(autoImages[autoImageIndex], 87, true); // (Placeholder values)
  } else {
    // Run the main menu and interactive logic
    handlemainmenu();
  }
}
