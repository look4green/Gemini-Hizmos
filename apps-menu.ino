/* we will be using this wone.
 * ------------------------------------------------
 * |       MULTI TOOL DEVICE: PENTESTING SYSTEM     |
 * ------------------------------------------------
 * | Target: ESP32-S3-N16R8                         |
 * | Display: 1.3-inch OLED (U8G2)                  |
 * | Control: 5 Buttons                             |
 * | Core Features: 10-point menu system            |
 * ------------------------------------------------
 */

// --- 1. LIBRARIES ---
#include <stdint.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

// Module Libraries
#include <nRF24L01.h>
#include <RF24.h>
#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include <IRremote.h>
#include <Adafruit_PN532.h>

// Connectivity & HID
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BleMouse.h>
#include "USB.h"
#include "USBHIDKeyboard.h"

// System Headers
#include <vector>
#include "esp_system.h"
// #include <TinyGPS++.h>       // ⚠️ Install and uncomment for GPS
// #include <cc1101.h>          // ⚠️ Install and uncomment for Sub-GHz

// Custom Headers (placeholders)
#include "animations.h"
#include "mainmenu.h"
#include "dolphinreactions.h" 


// --- 2. PIN DEFINITIONS ---

// I2C (OLED & NFC)
#define I2C_SDA     8
#define I2C_SCL     9

// Buttons (Input Pullup)
#define BTN_UP      4
#define BTN_DOWN    5
#define BTN_SELECT  6
#define BTN_BACK    7
#define BTN_LEFT    1
#define BTN_RIGHT   2
#define BUTTON_DEBOUNCE_MS 150

// IR (Sender/Receiver)
#define irsenderpin 41
#define irrecivepin 40

// NeoPixel LED
#define LED_PIN     48
#define LED_COUNT   1

// NFC (PN532)
#define PN532_IRQ   22
#define PN532_RESET 21

// FSPI Bus (nRF Modules - Primary High-Speed SPI)
#define NRF_SCK    18
#define NRF_MISO   16
#define NRF_MOSI   17

// nRF24 Module Pins (Shared FSPI)
#define CE1_PIN    10
#define CSN1_PIN   11
#define CE2_PIN    12
#define CSN2_PIN   13
#define CE3_PIN    20 
#define CSN3_PIN   19 

// HSPI Bus (SD Card - Secondary High-Speed SPI)
#define SD_SCK     14
#define SD_MISO    39
#define SD_MOSI    38
#define SD_CS      37

// CC1101 (Sub-GHz) Pins (Shares FSPI or secondary SPI)
#define CC1101_CS_PIN    33 
#define CC1101_GDO0_PIN  34 

// GPS Pins (Using UART2)
#define GPS_TX_PIN 15 
#define GPS_RX_PIN 42 

// GPIO Testing Pins
#define ANALOG_PIN 3


// --- 3. HARDWARE OBJECTS ---

// HID & BLE
USBHIDKeyboard Keyboard;
BleMouse mouse_ble("hizmos", "hizmos", 100);

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

// SPI Buses & RF24 Modules
SPIClass RADIO_SPI(FSPI);
SPIClass SD_SPI(HSPI);
RF24 radio1(CE1_PIN, CSN1_PIN);
RF24 radio2(CE2_PIN, CSN2_PIN);
RF24 radio3(CE3_PIN, CSN3_PIN); // Third module

// GPS Module 
// HardwareSerial GPSSerial(2); 
// TinyGPSPlus gps;


// --- 4. GLOBALS & SYSTEM STATE ---

int batteryPercent = 87;
bool sdOK = false; // Flag to track SD card status
bool inhizmosmenu = false;

// UI/System State
int mainMenuIndex = 1; 
const int MAX_MENU_INDEX = 10; 
const int MIN_MENU_INDEX = 1;

bool autoMode = true;
unsigned long lastImageChangeTime = 0;
unsigned long lastButtonPressTime = 0;
const unsigned long autoModeTimeout = 120000; 
int autoImageIndex = 0;
// Note: totalAutoImages/totalManualImages are defined as const int in the provided code.

// --- 5. FUNCTION PROTOTYPES (Simplified for menu management) ---
void setColor(uint8_t r, uint8_t g, uint8_t b);
void deactivateAllModules();
void displayImage(const uint8_t* image);
void runLoop(void (*func)());
void checksysdevices(); // System initialization check

// Menu Functions (Placeholder definitions for clean compilation)
void runWifiMenu() {}
void runBleMenu() {}
void runBadUsbMenu() {}
void runNfcMenu() {}
void runIrMenu() {}
void runSubGhzMenu() {}
void runRF24Menu() {}
void runGpioMenu() {}
void runSettingsMenu() {}
void runFilesMenu() {}
void claculaterloop() {}
void updateTimer() {}
void pomdorotimerloop() {}
void handleoscillomenu() {}
void snakeSetup() {}
void snakeLoop() {}
void readSDfiles() {}
void drawMenu() {}
void flasherloop() {}
void spacegame() {}
void handlePasswordMaker() {}

// --- 6. CORE UI LOGIC ---

// --- 6.1. Button Handling ---

// Waits for button release to prevent accidental double-clicks
void waitForRelease(uint8_t pin) {
  while (digitalRead(pin) == LOW) delay(5);
}

void handleButtonPress(int& selectedItem, int menuLength) {
    static unsigned long lastInputTime = 0;
    
    // Check for debounced input
    if (millis() - lastInputTime > BUTTON_DEBOUNCE_MS) {
        
        if (digitalRead(BTN_UP) == LOW) {
            waitForRelease(BTN_UP);
            selectedItem = (selectedItem - 1 + menuLength) % menuLength;
            lastInputTime = millis();
        } else if (digitalRead(BTN_DOWN) == LOW) {
            waitForRelease(BTN_DOWN);
            selectedItem = (selectedItem + 1) % menuLength;
            lastInputTime = millis();
        } else if (digitalRead(BTN_LEFT) == LOW) {
             waitForRelease(BTN_LEFT);
             lastInputTime = millis();
        } else if (digitalRead(BTN_RIGHT) == LOW) {
             waitForRelease(BTN_RIGHT);
             lastInputTime = millis();
        }
    }
}


// --- 6.2. Apps Menu (Feature #8) ---

void handleappsmenu() {
  const char* menuItems[] = {"calculator", "pomdoro timer", "oscilloscope", "snake game", "sd flasher tool", "spacecraft game", "pass generator"};
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3;

  static int selectedItem = 0;
  int scrollOffset = 0;

  // Handle Input
  handleButtonPress(selectedItem, menuLength);
  
  // Calculate Scroll Offset based on the currently selected item
  scrollOffset = constrain(selectedItem - visibleItems + 1, 0, menuLength - visibleItems);

  // Handle SELECT Button
  if (digitalRead(BTN_SELECT) == LOW) {
    waitForRelease(BTN_SELECT);
    switch (selectedItem) {
      case 0: runLoop(claculaterloop); break;
      case 1: updateTimer(); runLoop(pomdorotimerloop); break;
      case 2: runLoop(handleoscillomenu); break;
      case 3: snakeSetup(); runLoop(snakeLoop); break;
      case 4: // SD Flasher Tool
        if (sdOK) { 
            runLoop(flasherloop); 
        } else {
            // Temporary error message if SD card is not mounted
            u8g2.clearBuffer(); u8g2.drawStr(0, 20, "SD Card Missing!"); u8g2.sendBuffer();
            delay(2000); 
        }
        break;
      case 5: randomSeed(analogRead(ANALOG_PIN)); runLoop(spacegame); break;
      case 6: runLoop(handlePasswordMaker); break;
    }
  }

  // ===== Display Drawing =====
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tf); 

  // Draw Menu Items
  for (int i = 0; i < visibleItems; i++) {
    int menuIndex = i + scrollOffset;
    if (menuIndex >= menuLength) break;

    int y = i * 20 + 16;

    if (menuIndex == selectedItem) {
      u8g2.drawRBox(4, y - 12, 120, 16, 4); 
      u8g2.setDrawColor(0); 
      u8g2.drawStr(10, y, menuItems[menuIndex]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(10, y, menuItems[menuIndex]);
    }
  }

  // Draw Scroll Bar
  int barX = 124;
  int barHeight = 64;
  int dotY = map(selectedItem, 0, menuLength - 1, 0, barHeight);
  u8g2.drawFrame(barX, 0, 3, barHeight);
  u8g2.drawBox(barX, dotY, 3, 5); 
  
  u8g2.sendBuffer();
}


// --- 6.3. Main Menu (10 Features) ---

const char* mainMenuNames[] = {
    "1. WIFI ATTACKS", "2. BLE ATTACKS", "3. BAD USB", "4. NFC",
    "5. INFRARED", "6. SUB-GHZ", "7. 2.4GHZ RF", "8. APPS", 
    "9. SETTINGS", "10. FILES"
};

void drawMainMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 10);
    u8g2.print("--- MAIN MENU ---");

    int visibleItems = 6;
    int scrollOffset = constrain(mainMenuIndex - (visibleItems - 1), 1, MAX_MENU_INDEX - visibleItems + 1);

    for (int i = 0; i < visibleItems; i++) {
        int itemIndex = i + scrollOffset;
        if (itemIndex > MAX_MENU_INDEX) break;

        int displayY = 20 + i * 8;
        if (itemIndex == mainMenuIndex) {
            u8g2.drawRBox(0, displayY - 8, 128, 9, 0);
            u8g2.setDrawColor(0); 
            u8g2.setCursor(3, displayY);
            u8g2.print(">"); 
            u8g2.print(mainMenuNames[itemIndex - 1]);
            u8g2.setDrawColor(1);
        } else {
            u8g2.setCursor(3, displayY);
            u8g2.print(mainMenuNames[itemIndex - 1]);
        }
    }
    u8g2.sendBuffer();
}

void handlemainmenu() {
    // 1. Handle Input for main menu index
    int tempIndex = mainMenuIndex;
    handleButtonPress(tempIndex, MAX_MENU_INDEX);
    mainMenuIndex = tempIndex;

    // 2. Handle SELECT Button Action
    if (digitalRead(BTN_SELECT) == LOW) {
        waitForRelease(BTN_SELECT);
        
        switch (mainMenuIndex) {
            case 1: runLoop(runWifiMenu); break;
            case 2: runLoop(runBleMenu); break;
            case 3: runLoop(runBadUsbMenu); break;
            case 4: runLoop(runNfcMenu); break;
            case 5: runLoop(runIrMenu); break;
            case 6: runLoop(runSubGhzMenu); break;
            case 7: runLoop(runRF24Menu); break;
            case 8: runLoop(handleappsmenu); break; // Jump into the specialized apps menu
            case 9: runLoop(runSettingsMenu); break;
            case 10: runLoop(runFilesMenu); break;
            default: break;
        }
    }

    // 3. Draw the menu interface
    drawMainMenu();
}


// --- 7. SETUP & LOOP IMPLEMENTATION ---

void checksysdevices() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "--- STATUS CHECK ---");

  // 1. SD Card Check
  u8g2.drawStr(0, 20, "SD Card: ");
  if (SD.begin(SD_CS, SD_SPI)) {
    u8g2.drawStr(50, 20, "OK");
    sdOK = true;
  } else {
    u8g2.drawStr(50, 20, "FAIL");
    sdOK = false;
  }

  // 2. NFC Check
  u8g2.drawStr(0, 30, "NFC/PN532: ");
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (versiondata) {
    u8g2.drawStr(50, 30, "OK");
  } else {
    u8g2.drawStr(50, 30, "FAIL");
  }

  // 3. nRF24 Check (Simplified initialization check)
  u8g2.drawStr(0, 40, "RF24(1/2/3): ");
  if (radio1.begin(&RADIO_SPI) && radio2.begin(&RADIO_SPI) && radio3.begin(&RADIO_SPI)) {
    u8g2.drawStr(50, 40, "OK");
  } else {
    u8g2.drawStr(50, 40, "FAIL");
  }
  
  // (Placeholder for CC1101/GPS/IR checks)

  u8g2.sendBuffer();
}

void setup() {
  // 7.1. Pin Modes
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(WAVE_OUT_PIN, OUTPUT);
  pinMode(CC1101_CS_PIN, OUTPUT);
  pinMode(CC1101_GDO0_PIN, INPUT);
  
  // Set all CS/CE pins to output
  pinMode(SD_CS, OUTPUT);
  pinMode(CSN1_PIN, OUTPUT); pinMode(CSN2_PIN, OUTPUT); pinMode(CSN3_PIN, OUTPUT); 
  pinMode(CE1_PIN, OUTPUT); pinMode(CE2_PIN, OUTPUT); pinMode(CE3_PIN, OUTPUT); 

  deactivateAllModules();

  // 7.2. I2C Initialization
  Wire.begin(I2C_SDA, I2C_SCL); 
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);

  // 7.3. SPI Initialization
  RADIO_SPI.begin(NRF_SCK, NRF_MISO, NRF_MOSI); 
  SD_SPI.begin(SD_SCK, SD_MISO, SD_MOSI);     

  // 7.4. Other Modules
  irrecv.begin(irrecivepin);
  irsend.begin(irsenderpin);
  pixel.begin();
  pixel.setBrightness(80);
  pixel.show();
  USB.begin();
  Keyboard.begin();
  // GPSSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); // GPS

  // 7.5. Initial System Status Check
  drawstartinfo(); // Assuming this is an initial animated splash (from header)
  delay(1000);
  checksysdevices(); // Check and display module status
  delay(2000);
}

void loop() {
  // 7.6. Auto Mode Control
  if (digitalRead(BTN_BACK) == LOW) {
    waitForRelease(BTN_BACK);
    autoMode = true;
  }
  
  if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW || digitalRead(BTN_SELECT) == LOW) {
      // Any interaction forces the device out of auto mode
      autoMode = false;
  }

  // 7.7. Main Loop Execution
  if (!autoMode && millis() - lastButtonPressTime > autoModeTimeout) {
      autoMode = true;
  }
  
  setColor(0, 0, 0); // Reset LED unless a notification is active

  if (autoMode) {
    // Run the animation loop
    if (millis() - lastImageChangeTime > 300) {
      autoImageIndex = (autoImageIndex + 1) % totalAutoImages;
      lastImageChangeTime = millis();
    }
    // displaymainanim is assumed to handle the animation using the indexed image
    // displaymainanim(autoImages[autoImageIndex], batteryPercent, sdOK);
  } else {
    // Run the main menu and interactive logic
    handlemainmenu();
  }
}
