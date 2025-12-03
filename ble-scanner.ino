#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: Assuming external definitions for BTN_*, u8g2, BLEDevice, 
// and the debounce utility: extern bool checkButtonPress(...)

// --- Global State ---
// Assuming these are defined elsewhere and accessible
// extern std::vector<blescanner_Device> blescanner_devices;
// extern int blescanner_selectedIndex;
// extern BLEScan* blescanner_pBLEScan;

// --- Non-Blocking Status State ---
static bool showDetailsScreen = false;
static unsigned long detailScreenStartTime = 0;
const unsigned long DETAIL_DISPLAY_DURATION = 3000; // 3 seconds display time

// --- Scanning State ---
static bool isScanning = false;
static unsigned long scanStartTime = 0;
const unsigned long SCAN_DURATION_MS = 5000;

// Debounce state placeholders (must be defined globally or passed)
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnSelect = HIGH;
static bool lastBtnBack = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long selectPressTime = 0;
static unsigned long backPressTime = 0;

// External function assumed
extern void blescanner_drawMenu();

// --- Non-Blocking BLE Scan Initialization ---
void blescanner_startScan() {
  // Clean up previous results
  blescanner_devices.clear();
  blescanner_selectedIndex = 0;
  
  // Setup scanner (should run once in setup/init, but here for completeness)
  BLEDevice::init("");
  blescanner_pBLEScan = BLEDevice::getScan();
  blescanner_pBLEScan->setAdvertisedDeviceCallbacks(new blescanner_AdvertisedDeviceCallbacks(), false);
  blescanner_pBLEScan->setActiveScan(true);

  // Start the scan asynchronously (duration is managed by main loop)
  blescanner_pBLEScan->start(0, false); // Start scan indefinitely
  scanStartTime = millis();
  isScanning = true;
}

// --- Detail Screen Handler (Non-Blocking) ---
void blescanner_showDetails(const blescanner_Device& dev) {
  // Draw once and set timer
  blescanner_drawDeviceDetails(dev);
  detailScreenStartTime = millis();
  showDetailsScreen = true;
}

void blescanner_handleDetailScreen() {
  // Check if the status time is up
  if (millis() - detailScreenStartTime >= DETAIL_DISPLAY_DURATION) {
    showDetailsScreen = false; // Turn off info screen
    blescanner_drawMenu(); // Immediately redraw the menu
    return;
  }
  // Optional: Allow BACK button to exit details screen early
  if (checkButtonPress(BTN_BACK, lastBtnBack, backPressTime)) {
    showDetailsScreen = false;
    blescanner_drawMenu();
    return;
  }
  // Note: Detail screen is already drawn in blescanner_showDetails, 
  // no need to redraw every loop unless status changes.
}

void blescanner() {
  // --- STATE 1: VIEWING DEVICE DETAILS (Highest Priority) ---
  if (showDetailsScreen) {
    blescanner_handleDetailScreen();
    return;
  }

  // --- STATE 2: SCANNING IN PROGRESS ---
  if (isScanning) {
    // Draw a simple scanning progress screen
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x14_tr);
    u8g2.drawStr(0, 30, "Scanning...");
    u8g2.drawStr(0, 48, String(blescanner_devices.size()) + " found.");
    u8g2.sendBuffer();

    // Check if scan duration is over
    if (millis() - scanStartTime >= SCAN_DURATION_MS) {
      blescanner_pBLEScan->stop();
      isScanning = false;
      blescanner_drawMenu(); // Switch to menu view
    }
    
    // Allow BACK to interrupt scan early
    if (checkButtonPress(BTN_BACK, lastBtnBack, backPressTime)) {
      blescanner_pBLEScan->stop();
      isScanning = false;
      blescanner_drawMenu();
    }
    return;
  }
  
  // --- STATE 3: BROWSING MENU (Only runs if not scanning or showing details) ---

  // DOWN Navigation (Non-blocking)
  if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
    if (blescanner_selectedIndex < (int)blescanner_devices.size() - 1) {
      blescanner_selectedIndex++;
      blescanner_drawMenu(); // Redraw only on change
    }
  } 
  // UP Navigation (Non-blocking)
  else if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
    if (blescanner_selectedIndex > 0) {
      blescanner_selectedIndex--;
      blescanner_drawMenu(); // Redraw only on change
    }
  } 
  // SELECT Button (Show Details)
  else if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    if (!blescanner_devices.empty()) {
      blescanner_showDetails(blescanner_devices[blescanner_selectedIndex]);
    }
  } 
  // BACK Button (Start Rescan)
  else if (checkButtonPress(BTN_BACK, lastBtnBack, backPressTime)) {
    blescanner_startScan();
  }
}

