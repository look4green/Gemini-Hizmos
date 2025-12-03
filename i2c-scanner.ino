#include <Arduino.h>
#include <Wire.h> 
#include <U8g2lib.h> 

// --- Global State and Constants ---
uint8_t i2cFoundAddresses[32]; // Max 32 devices (for safety, though 127 addresses exist)
uint8_t i2cDeviceCount = 0;
uint8_t i2cSelectedIndex = 0;

// Device Info Screen State
static bool showInfoScreen = false;
static uint8_t infoAddress = 0;
static unsigned long infoScreenStartTime = 0;
const unsigned long INFO_DISPLAY_DURATION = 1500; // Duration for status screen

// Debounce state placeholders (must be defined globally or passed)
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnSelect = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long selectPressTime = 0;

// Placeholder for external functions/constants
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2; 
extern int BTN_UP, BTN_DOWN, BTN_SELECT;
extern int I2C_SDA, I2C_SCL;
extern bool checkButtonPress(int pin, bool &lastState, unsigned long &pressTime); 
const char* identifyDevice(uint8_t address);
void i2cDrawMenu();

void setupi2c(){
  // Assume I2C_SDA and I2C_SCL are defined globally
  Wire.begin(I2C_SDA, I2C_SCL);
  i2cScan();
}

void i2cScan() {
  // NOTE: This function is blocking, but it is acceptable to run once during setup.
  i2cDeviceCount = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    // Wire.endTransmission() returns 0 on success
    if (Wire.endTransmission() == 0) {
      i2cFoundAddresses[i2cDeviceCount++] = address;
      // Removed unnecessary delay(5);
    }
    // If i2cDeviceCount hits 32 (max array size), stop scanning
    if (i2cDeviceCount >= 32) break; 
  }
}

void i2cShowDeviceInfo_NonBlocking() {
  // Check if the status time is up
  if (millis() - infoScreenStartTime >= INFO_DISPLAY_DURATION) {
    showInfoScreen = false; // Turn off info screen, return to menu
    return;
  }
  
  // Draw the info screen
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.setCursor(0, 12);
  u8g2.print("Address: 0x");
  u8g2.print(infoAddress, HEX);
  u8g2.setCursor(0, 28);
  u8g2.print("Device:");
  u8g2.setCursor(0, 44);
  u8g2.print(identifyDevice(infoAddress));
  u8g2.sendBuffer();
  
  // Note: No input checking is done here, as the main loop will handle exiting 
  // this state if a dedicated "BACK" button is pressed, or if the timer expires.
}

/**
 * @brief Transitions to the device info state.
 */
void startShowDeviceInfo(uint8_t address) {
  infoAddress = address;
  infoScreenStartTime = millis();
  showInfoScreen = true;
}

void i2cscanner() {
  // --- STATE 1: Showing Device Info ---
  if (showInfoScreen) {
    i2cShowDeviceInfo_NonBlocking();
    return; // Do not process menu inputs or draw menu while showing info
  }
  
  // --- STATE 2: Menu Navigation ---
  if (i2cDeviceCount > 0) {
    // Navigation: UP (Non-blocking)
    if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
      i2cSelectedIndex = (i2cSelectedIndex == 0) ? i2cDeviceCount - 1 : i2cSelectedIndex - 1;
    }
    
    // Navigation: DOWN (Non-blocking)
    if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
      i2cSelectedIndex = (i2cSelectedIndex + 1) % i2cDeviceCount;
    }

    // Action: SELECT (Non-blocking)
    if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
      startShowDeviceInfo(i2cFoundAddresses[i2cSelectedIndex]);
      return; // Transition immediately to info screen state
    }
    
    // Draw menu if inputs were processed or if no devices found
    i2cDrawMenu();

  } else {
    // Draw message if no devices were found
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x14_tr);
    u8g2.drawStr(0, 30, "No I2C Devices Found");
    u8g2.drawStr(0, 50, "Scan Again?");
    u8g2.sendBuffer();
    
    // Optionally, add logic here to re-run i2cScan() on a button press
    if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
        i2cScan(); // Re-scan the bus
    }
  }
}


void i2cDrawMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);

  uint8_t itemsPerPage = 5;
  // Calculate the starting index for the current page
  uint8_t start = (i2cSelectedIndex / itemsPerPage) * itemsPerPage; 

  for (uint8_t i = 0; i < itemsPerPage; i++) {
    uint8_t actualIndex = start + i;
    // Stop if we run out of devices to draw
    if (actualIndex >= i2cDeviceCount) break; 
    
    int y = i * 13;

    u8g2.drawFrame(0, y, 128, 13);

    char line[32];
    // Note: Cleaned up the formatting for better readability
    sprintf(line, "0x%02X %s", i2cFoundAddresses[actualIndex], identifyDevice(i2cFoundAddresses[actualIndex]));

    if (actualIndex == i2cSelectedIndex) {
      u8g2.drawRBox(3, y + 2, 122, 9, 3);
      u8g2.setDrawColor(0); // Black text
      u8g2.setCursor(6, y + 10);
      u8g2.print(line);
      u8g2.setDrawColor(1); // Back to white text
    } else {
      u8g2.setCursor(6, y + 10);
      u8g2.print(line);
    }
  }

  u8g2.sendBuffer();
}

