// ==============================================================================
// 🤖 AI-GENERATED ESP32 MULTI-TOOL FIRMWARE FOUNDATION
// Based on detailed prompt for a complex, feature-rich device.
// ==============================================================================

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
// Custom Headers (Placeholders - ensure these files exist in your project)
#include "animations.h"
#include "mainmenu.h"
#include "dolphinreactions.h" 


// --- 2. PIN DEFINITIONS ---

// I2C (OLED & NFC)
#define I2C_SDA      8
#define I2C_SCL      9

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

// Placeholder Pin (e.g., for randomSeed)
#define ANALOG_PIN 3


// --- 3. HARDWARE OBJECTS ---

// HID & BLE
USBHIDKeyboard Keyboard;
BleMouse mouse_ble("hizmos", "hizmos", 100);

// Display (128x64 OLED via HW I2C)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// NeoPixel
Adafruit_NeoPixel pixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// NFC/RFID
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

// IR
IRrecv irrecv(irrecivepin);
IRsend irsend(irsenderpin);
// decode_results results; // Defined in the original code, but often global in IRremote

// SPI Buses & RF24 Modules
SPIClass RADIO_SPI(FSPI); // Define FSPI bus
SPIClass SD_SPI(HSPI);   // Define HSPI bus
RF24 radio1(CE1_PIN, CSN1_PIN);
RF24 radio2(CE2_PIN, CSN2_PIN);
RF24 radio3(CE3_PIN, CSN3_PIN); // Third module


// --- 4. GLOBALS & SYSTEM STATE ---

int batteryPercent = 87;
bool sdOK = false; // Flag to track SD card status
bool inhizmosmenu = false;

// UI/System State
int mainMenuIndex = 1; 
const int MAX_MENU_INDEX = 10; 
const int MIN_MENU_INDEX = 1; // Note: Main menu indices are 1-10

// Core State Management
bool autoMode = true;
unsigned long lastImageChangeTime = 0;
unsigned long lastButtonPressTime = 0;
const unsigned long autoModeTimeout = 120000; // 120 seconds for screensaver

// Placeholder for animation index
int autoImageIndex = 0;
// Note: totalAutoImages/totalManualImages are defined as const int in the provided code.


// --- 5. FUNCTION PROTOTYPES (Simplified for menu management) ---
// Note: These need to be defined in your main code or included headers.
void runLoop(void (*func)());
void deactivateAllModules();
void checksysdevices(); 
void drawstartinfo();
void displaymainanim(int index, int battery, bool sdStatus);
void setColor(uint8_t r, uint8_t g, uint8_t b);

// Menu Handlers
void handlemainmenu();
void handleappsmenu();

// Module Runners (Empty Prototypes as requested)
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
void flasherloop() {}
void spacegame() {}
void handlePasswordMaker() {}


// --- 6. CORE UI LOGIC ---

// --- 6.1. Generic Input Handling ---

// Waits for button release to prevent accidental double-clicks
void waitForRelease(uint8_t pin) {
  while (digitalRead(pin) == LOW) delay(5);
}

// Generic reusable function for scrolling menus
void handleButtonPress(int& selectedItem, int menuLength) {
    static unsigned long lastInputTime = 0;
    
    // Check for debounced input
    if (millis() - lastInputTime > BUTTON_DEBOUNCE_MS) {
        
        if (digitalRead(BTN_UP) == LOW) {
            waitForRelease(BTN_UP);
            // Wrap-around scrolling logic for UP
            selectedItem = (selectedItem - 1 + menuLength) % menuLength;
            lastInputTime = millis();
        } else if (digitalRead(BTN_DOWN) == LOW) {
            waitForRelease(BTN_DOWN);
            // Wrap-around scrolling logic for DOWN
            selectedItem = (selectedItem + 1) % menuLength;
            lastInputTime = millis();
        } 
        // Note: BTN_LEFT/BTN_RIGHT logic often handled contextually
    }
    // Update the lastButtonPressTime global for Auto Mode timeout
    if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW || digitalRead(BTN_SELECT) == LOW || digitalRead(BTN_BACK) == LOW) {
        lastButtonPressTime = millis();
    }
}


// --- 6.2. Apps Menu (Nested Menu Logic) ---

void handleappsmenu() {
  const char* menuItems[] = {"calculator", "pomdoro timer", "oscilloscope", "snake game", "sd flasher tool", "spacecraft game", "pass generator"};
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3;

  static int selectedItem = 0;
  int scrollOffset = 0;

  // Handle Input (Menu index is 0-based for this function)
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
      case 4: // SD Flasher Tool - Conditional Check
        if (sdOK) { 
             runLoop(flasherloop); 
        } else {
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
  // 

  // Draw Menu Items
  for (int i = 0; i < visibleItems; i++) {
    int menuIndex = i + scrollOffset;
    if (menuIndex >= menuLength) break;

    int y = i * 20 + 16;

    if (menuIndex == selectedItem) {
      u8g2.drawRBox(4, y - 12, 120, 16, 4); // Highlighted box
      u8g2.setDrawColor(0); // Set color to black for text over highlight
      u8g2.drawStr(10, y, menuItems[menuIndex]);
      u8g2.setDrawColor(1); // Reset color to white for standard drawing
    } else {
      u8g2.drawStr(10, y, menuItems[menuIndex]);
    }
  }

  // Draw Scroll Bar (Advanced Logic)
  int barX = 124;
  int barHeight = 64;
  // Map selected item index to a pixel position within the scroll bar
  int dotY = map(selectedItem, 0, menuLength - 1, 0, barHeight);
  u8g2.drawFrame(barX, 0, 3, barHeight);
  u8g2.drawBox(barX, dotY, 3, 5); 
  
  u8g2.sendBuffer();
}


// --- 6.3. Main Menu (Top-Level Logic) ---

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
    // Calculate scroll offset (menuIndex is 1-based here)
    int scrollOffset = constrain(mainMenuIndex - (visibleItems - 1), 1, MAX_MENU_INDEX - visibleItems + 1);

    for (int i = 0; i < visibleItems; i++) {
        int itemIndex = i + scrollOffset;
        if (itemIndex > MAX_MENU_INDEX) break;

        int displayY = 20 + i * 8;
        if (itemIndex == mainMenuIndex) {
            u8g2.drawRBox(0, displayY - 8, 128, 9, 0); // Highlighted box
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
    // 1. Handle Input for main menu index (Using 1-based index: 1 to 10)
    int tempIndex = mainMenuIndex;
    handleButtonPress(tempIndex, MAX_MENU_INDEX);
    
    // The handleButtonPress function returns 0-based. Convert back to 1-based if necessary,
    // or adjust the handleButtonPress logic. For simplicity, we assume main menu is 1-10.
    if (digitalRead(BTN_UP) == LOW) {
        waitForRelease(BTN_UP);
        mainMenuIndex = (mainMenuIndex - 1 < MIN_MENU_INDEX) ? MAX_MENU_INDEX : mainMenuIndex - 1;
    } else if (digitalRead(BTN_DOWN) == LOW) {
        waitForRelease(BTN_DOWN);
        mainMenuIndex = (mainMenuIndex + 1 > MAX_MENU_INDEX) ? MIN_MENU_INDEX : mainMenuIndex + 1;
    }


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

// Placeholder function for system checks (as per original code structure)
void checksysdevices() {
    // Implementation needed here...
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "SYSTEM CHECK...");
    u8g2.sendBuffer();
    // Simplified SD check for setup
    sdOK = SD.begin(SD_CS, SD_SPI);
    
    // Placeholder for other checks (NFC, RF24, etc.)
}

// Placeholder function for module deactivation
void deactivateAllModules() {
    // Set all CS/CE pins high to deactivate modules initially
    digitalWrite(SD_CS, HIGH);
    digitalWrite(CSN1_PIN, HIGH); digitalWrite(CSN2_PIN, HIGH); digitalWrite(CSN3_PIN, HIGH); 
    digitalWrite(CE1_PIN, LOW);  digitalWrite(CE2_PIN, LOW);  digitalWrite(CE3_PIN, LOW); 
}

void setup() {
    // 7.1. Pin Modes
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);
    pinMode(BTN_BACK, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    
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
    
    // 7.5. Initial System Status Check
    // drawstartinfo(); // Assuming this is an initial animated splash
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
    
    // Any interaction forces the device out of auto mode
    if (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW || digitalRead(BTN_SELECT) == LOW) {
        autoMode = false;
        lastButtonPressTime = millis();
    }

    // 7.7. Main Loop Execution
    if (!autoMode && millis() - lastButtonPressTime > autoModeTimeout) {
        autoMode = true;
    }
    
    setColor(0, 0, 0); // Placeholder: Reset LED

    if (autoMode) {
        // Placeholder for Animation Loop
        if (millis() - lastImageChangeTime > 300) {
            // Note: totalAutoImages must be defined globally
            // autoImageIndex = (autoImageIndex + 1) % totalAutoImages; 
            lastImageChangeTime = millis();
        }
        // displaymainanim(autoImages[autoImageIndex], batteryPercent, sdOK); // Placeholder
    } else {
        // Run the main menu and interactive logic
        handlemainmenu();
    }
}
