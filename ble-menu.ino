#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: Assuming external definitions for BTN_*, u8g2, runLoop, 
// blescanner, blemouse, loading, and the debounce utility: 
// extern bool checkButtonPress(...)

// Debounce state placeholders (must be defined globally or passed)
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnSelect = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long selectPressTime = 0;

// Menu state variables
const char* menuItems[] = {"BLE SCANNER","BLE MOUSE", "BLE KEYBOARD", "BLE SCRIPT"};
const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
const int visibleItems = 3;

static int selectedItem = 0;
static int scrollOffset = 0;

// External function assumed:
extern void blescanner_startScan(); 
extern void mouse_ble_begin(); // Assuming the mouse lib has a begin function

void handleblemenu() {
  
  // ===== التعامل مع الأزرار (Non-blocking) =====
  
  // UP Navigation
  if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
    selectedItem--;
    if (selectedItem < 0) selectedItem = menuLength - 1;
    scrollOffset = constrain(selectedItem - visibleItems + 1, 0, menuLength - visibleItems);
  }

  // DOWN Navigation
  if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
    selectedItem++;
    if (selectedItem >= menuLength) selectedItem = 0;
    // Adjust scroll offset to keep selected item within visible window
    if (selectedItem >= scrollOffset + visibleItems) {
      scrollOffset++;
    } else if (selectedItem < scrollOffset) {
      scrollOffset--;
    }
    // Ensure scrollOffset never goes below 0
    scrollOffset = constrain(scrollOffset, 0, menuLength - visibleItems);
  }

  // SELECT Action
  if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    switch (selectedItem) {
      case 0: // BLE SCANNER
        // Start the asynchronous scan process and switch to the scanner loop
        blescanner_startScan(); // Initialize the scan state (non-blocking)
        runLoop(blescanner); // Enter the blescanner loop handler
        break;
      case 1: // BLE MOUSE
        mouse_ble.begin();
        runLoop(blemouse);
        break;
      case 2: // BLE KEYBOARD
        // runLoop(blekeyboard); // Assuming a blekeyboard function exists
        runLoop(loading);
        break;
      case 3: // BLE SCRIPT
        // runLoop(blescript); // Assuming a blescript function exists
        runLoop(loading);
        break;
      // FIX: Case 4 removed as it is out of bounds (menuLength is 4, max index is 3)
    }
    // No need to update lastInputTime, checkButtonPress handles timing.
  }


  // ===== عرض الشاشة (Drawing UI) =====
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tf); 

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

  // ===== شريط التمرير (Scroll Bar) =====
  int barX = 124;
  // Calculate spacing based on the full height of the display area (e.g., 60 pixels)
  int scrollAreaHeight = 60; 
  int spacing = scrollAreaHeight / menuLength; 

  for (int i = 0; i < menuLength; i++) {
    int dotY = i * spacing + 5; // Start dot slightly below the top
    if (i == selectedItem) {
      u8g2.drawBox(barX, dotY - 3, 3, 6); // Highlighted dot (box)
    } else {
      u8g2.drawPixel(barX + 1, dotY); // Unselected dot
    }
  }

  u8g2.sendBuffer();
}

// NOTE: The inline drawing logic for case 0 is removed as it freezes the UI.
// The custom "scanning" screen should now be handled asynchronously within the blescanner function itself.

