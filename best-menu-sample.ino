#include <Arduino.h>
#include <U8g2lib.h>

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// Define placeholders for the U8G2 object and button pins
// NOTE: Replace U8G2_... with your specific display model and configuration.
// extern U8G2 u8g2; 
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_SELECT 7
#define BTN_BACK 8 

// --- 2. FORWARD DECLARATIONS (Required for compilation) ---

// Utility functions
void runLoop(void (*handler)());
void waitForRelease(uint8_t pin);
void handlemainmenu();

// Target functions (your NFC/Sample Menu actions)
void readNFCTag();
void writeNFCData();
void showSavedTags();
void emulateNFCTag();
void startP2PMode();

// --- 3. UTILITY IMPLEMENTATION ---

// Simple placeholder for runLoop (your system's main state machine)
void runLoop(void (*handler)()) {
  // In a real system, this changes the active function in the main loop.
  // We use this to return control after an action is complete.
  handler();
}

/**
 * @brief Waits until the specified button is released.
 * @param pin The button pin to check (e.g., BTN_UP, BTN_DOWN, BTN_SELECT).
 */
void waitForRelease(uint8_t pin) {
  // Assuming buttons are pulled up (HIGH) and LOW when pressed
  while (digitalRead(pin) == LOW) {
    delay(5); // Debounce delay while held
  }
}

// --- 4. THE CORRECTED handlesamplemenu() FUNCTION ---

void handlesamplemenu() {
  const char* menuItems[] = {"Read tag", "Write Data", "Saved Tags", "Emulate Tag", "P2P comunication"};
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3; 

  static int selectedItem = 0;
  static int scrollOffset = 0;

  // The time-based debounce logic (`lastInputTime`) is removed, 
  // replaced by the more robust `waitForRelease`.

  // ===== التعامل مع الأزرار (Input Handling) =====

  if (digitalRead(BTN_UP) == LOW) {
    waitForRelease(BTN_UP); 
    selectedItem--;
    if (selectedItem < 0) selectedItem = menuLength - 1; // Wrap around to end
    
    // Adjust scroll offset to keep selectedItem visible (ideally centered or at top)
    if (selectedItem < scrollOffset) {
        scrollOffset = selectedItem; // Scroll up immediately
    } else if (selectedItem == menuLength - 1) {
        // If wrapping from 0 to last, update scrollOffset to show the last items
        scrollOffset = constrain(menuLength - visibleItems, 0, menuLength);
    }
    // Re-constrain to prevent going out of bounds
    scrollOffset = constrain(scrollOffset, 0, menuLength - visibleItems); 
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    waitForRelease(BTN_DOWN); 
    selectedItem++;
    if (selectedItem >= menuLength) selectedItem = 0; // Wrap around to start
    
    // Adjust scroll offset to keep selectedItem visible (ideally centered or at bottom)
    if (selectedItem >= scrollOffset + visibleItems) {
        scrollOffset = selectedItem - visibleItems + 1; // Scroll down immediately
    } else if (selectedItem == 0) {
        // If wrapping from last to 0, reset scrollOffset to show first items
        scrollOffset = 0; 
    }
    // Re-constrain to prevent going out of bounds
    scrollOffset = constrain(scrollOffset, 0, menuLength - visibleItems); 
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    waitForRelease(BTN_SELECT); 
    switch (selectedItem) {
      case 0: runLoop(readNFCTag); break;
      case 1: runLoop(writeNFCData); break;
      case 2: runLoop(showSavedTags); break;
      case 3: runLoop(emulateNFCTag); break;
      case 4: runLoop(startP2PMode); break;
    }
  }

  // Allow exiting the menu
  if (digitalRead(BTN_BACK) == LOW) {
    waitForRelease(BTN_BACK);
    runLoop(handlemainmenu); 
    return;
  }

  // ===== عرض القائمة على الشاشة (Display Rendering) =====
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tf);

  // Draw Menu Items
  for (int i = 0; i < visibleItems; i++) {
    int menuIndex = i + scrollOffset;
    if (menuIndex >= menuLength) break;

    // Y position for text baseline (16, 36, 56, etc.)
    int y = i * 20 + 16; 

    if (menuIndex == selectedItem) {
      u8g2.drawRBox(4, y - 12, 120, 16, 4); // Highlight box
      u8g2.setDrawColor(0); // Black text on white box
      u8g2.drawStr(10, y, menuItems[menuIndex]);
      u8g2.setDrawColor(1); // Reset draw color to white
    } else {
      u8g2.drawStr(10, y, menuItems[menuIndex]);
    }
  }

  // ===== شريط التمرير الجانبي (Scroll Bar) =====
  // Using a proportional box for a better scroll indication
  int barX = 124;
  int totalHeight = 64; 
  
  // Calculate the size of the marker based on menu length
  int markerHeight = totalHeight / menuLength; 
  if (markerHeight < 3) markerHeight = 3; // Minimum size for visibility

  // Position the marker based on the selected item's index
  int markerY = map(selectedItem, 0, menuLength - 1, 0, totalHeight - markerHeight);
  
  // Draw the scroll track and the marker
  u8g2.drawVLine(barX + 1, 0, totalHeight); // Scroll track
  u8g2.drawBox(barX, markerY, 3, markerHeight); // Proportional marker

  u8g2.sendBuffer(); 
}
