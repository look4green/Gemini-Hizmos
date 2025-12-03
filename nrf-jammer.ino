#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: Assuming external definitions for BTN_*, radio1/2, etc.

// --- Debounce Constants and Global Trackers (from previous fix) ---
const unsigned long debounceDelay = 150; 
static bool lastBtnLeft = HIGH;
static bool lastBtnRight = HIGH;
static bool lastBtnSelect = HIGH;
static bool lastBtnBack = HIGH;
static unsigned long leftPressTime = 0;
static unsigned long rightPressTime = 0;
static unsigned long selectPressTime = 0;
static unsigned long backPressTime = 0;

bool checkButtonPress(int pin, bool &lastState, unsigned long &pressTime); 
// Assumed definition for checkButtonPress exists from the previous context

// --- Jammer-Specific Constants and State ---
#define JAM_DURATION 100 // Duration (ms) to jam each channel before switching
#define STATUS_DISPLAY_DURATION 1500 // Time (ms) to show "Done" message

// Jamming State Machine Variables
static bool jammingActive = false;
static int jamStartChannel = 0;
static int jamEndChannel = 0;
static int currentJamChannel = 0;
static unsigned long lastJamChannelSwitch = 0;
static String jamLabel = "";
static bool statusScreenActive = false;
static unsigned long statusScreenStartTime = 0;
// ---

namespace nrf {
  bool stopJamming = false;
  
  // Adjusted channel numbers for common use bands in nRF channel space (0-125)
  const char* menuItems[] = {
    "Bluetooth", // Range 0-78 
    "Zigbee",    // Range 15-26 (Channels 11-26 mapped)
    "Wi-Fi",     // Range 10-84 (Covers Wi-Fi 2412-2472 MHz, 1-13)
    "Drones",    // Range 40-70 (Common control band)
    "All"
  };

  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  int currentPage = 0;
}

// Generate random data outside the loop to save time
byte data1[32], data2[32]; 

void initializeJamData() {
  for (int i = 0; i < 32; i++) {
    data1[i] = random(0, 256);
    data2[i] = random(0, 256);
  }
}


/**
 * @brief Performs one non-blocking step of the jamming sequence.
 */
void jamChannels_NonBlocking() {
  unsigned long now = millis();

  // 1. Check if it's time to switch channels
  if (now - lastJamChannelSwitch >= JAM_DURATION) {
    lastJamChannelSwitch = now;

    // 2. Check for completion or manual stop
    if (currentJamChannel > jamEndChannel || nrf::stopJamming) {
      jammingActive = false;
      statusScreenActive = true;
      statusScreenStartTime = now;
      return; // Jamming sequence complete/stopped
    }

    // 3. Update Status Screen (Non-blocking status update)
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 10, "Jamming:");
    u8g2.setCursor(60, 10);
    u8g2.print(jamLabel.c_str());
    u8g2.setCursor(0, 30);
    u8g2.print("Channel: ");
    u8g2.print(currentJamChannel);
    u8g2.sendBuffer();
    
    // 4. Set the new channel for the next pulse
    radio1.setChannel(currentJamChannel);
    radio2.setChannel(currentJamChannel);

    currentJamChannel++;
  }
  
  // 5. Send the jamming pulse (happens every loop iteration while on channel)
  // This uses microsecond delays, which are acceptable for hardware operations
  // but we avoid longer delays.
  radio1.stopListening();
  radio1.write(data1, sizeof(data1));
  delayMicroseconds(100);

  radio2.stopListening();
  radio2.write(data2, sizeof(data2));
  delayMicroseconds(100);

  // 6. Check for immediate stop (BTN_BACK)
  if (digitalRead(BTN_BACK) == LOW) {
    nrf::stopJamming = true;
  }
}


/**
 * @brief Sets up the parameters and starts the non-blocking jam sequence.
 */
void startJamSequence(const char* label, int startCh, int endCh) {
  initializeJamData();
  nrf::stopJamming = false;
  jammingActive = true;
  jamLabel = label;
  jamStartChannel = startCh;
  jamEndChannel = endCh;
  currentJamChannel = startCh;
  lastJamChannelSwitch = millis() - JAM_DURATION; // Force immediate start
}


void handleSelection(int item) {
  switch (item) {
    case 0: startJamSequence("Bluetooth", 0, 78); break;
    case 1: startJamSequence("Zigbee", 15, 26); break;
    case 2: startJamSequence("Wi-Fi", 10, 84); break;
    case 3: startJamSequence("Drones", 40, 70); break;
    case 4: startJamSequence("All", 0, 84); break;
  }
}

void drawMenuPage(int index) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_logisoso16_tr);

  // Title centered
  int strWidth = u8g2.getStrWidth(nrf::menuItems[index]);
  u8g2.drawStr((128 - strWidth) / 2, 35, nrf::menuItems[index]);

  // Bottom indicator
  for (int i = 0; i < nrf::menuLength; i++) {
    int x = 20 + i * 15;
    if (i == index)
      u8g2.drawDisc(x, 60, 3);  // filled
    else
      u8g2.drawCircle(x, 60, 3); // hollow
  }

  u8g2.sendBuffer();
}


void nrfjammer() {
  
  // --- STATE 1: Jamming Active ---
  if (jammingActive) {
    jamChannels_NonBlocking();
    return; // Do nothing else while jamming
  }
  
  // --- STATE 2: Show Status Screen (Done/Stopped) ---
  if (statusScreenActive) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, nrf::stopJamming ? "Stopped by user" : "Jamming Done");
    u8g2.sendBuffer();
    
    // Non-blocking timer for status screen
    if (millis() - statusScreenStartTime >= STATUS_DISPLAY_DURATION) {
      statusScreenActive = false; // Return to menu
    }
    return;
  }
  
  // --- STATE 3: Menu Navigation ---
  
  // Left Swipe
  if (checkButtonPress(BTN_LEFT, lastBtnLeft, leftPressTime)) {
    nrf::currentPage = (nrf::currentPage - 1 + nrf::menuLength) % nrf::menuLength;
    drawMenuPage(nrf::currentPage);
  }

  // Right Swipe
  if (checkButtonPress(BTN_RIGHT, lastBtnRight, rightPressTime)) {
    nrf::currentPage = (nrf::currentPage + 1) % nrf::menuLength;
    drawMenuPage(nrf::currentPage);
  }

  // Select Item
  if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    handleSelection(nrf::currentPage); // Starts the jam sequence (sets jammingActive = true)
    return; // Exit to immediately begin jamming state
  }
  
  // If no input was pressed, ensure the current menu page is drawn (e.g., on entry)
  // This helps maintain the screen if the calling loop is not drawing it constantly.
  // drawMenuPage(nrf::currentPage); 
}
