#include <Arduino.h>
#include <U8g2lib.h>

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
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
void handlemainmenu(); // Function to return to

// Target functions (your Sub-GHz actions)
void subghz_read();
void subghz_read_raw();
void subghz_frequency_analyzer();
void subghz_jammer();
void subghz_saved_signals();


// --- 3. UTILITY IMPLEMENTATION ---

// Simple placeholder for runLoop (your system's main state machine)
void runLoop(void (*handler)()) {
  handler();
}

/**
 * @brief Waits until the specified button is released.
 * Prevents multiple selections/scrolls from a single, long button press.
 * @param pin The button pin to check.
 */
void waitForRelease(uint8_t pin) {
  // Assuming buttons are pulled up (HIGH) and LOW when pressed
  while (digitalRead(pin) == LOW) {
    delay(5); // Debounce delay while held
  }
}

// --- 4. THE CORRECTED handlesubghzmenu() FUNCTION ---

void handlesubghzmenu() {
  // Corrected menu items from the original (index 3 and 4 were mixed up in the original switch)
  const char* menuItems[] = {"READ", "READ RAW", "FREQUENCY ANALYZER", "JAMMER", "SAVED SIGNALS"};
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3; 

  static int selectedItem = 0;
  static int scrollOffset = 0;

  // Time-based debounce logic is removed and replaced by waitForRelease.

  // ===== التعامل مع الأزرار (Input Handling) =====

  if (digitalRead(BTN_UP) == LOW) {
    waitForRelease(BTN_UP); 
    selectedItem--;
    if (selectedItem < 0) selectedItem = menuLength - 1; // Wrap around
    
    // Scroll offset logic to keep selected item visible
    if (selectedItem < scrollOffset) {
        scrollOffset = selectedItem; 
    } else if (selectedItem == menuLength - 1) {
        scrollOffset = constrain(menuLength - visibleItems, 0, menuLength);
    }
    scrollOffset = constrain(scrollOffset, 0, menuLength - visibleItems); 
  }

  if (digitalRead(BTN_DOWN) == LOW) {
    waitForRelease(BTN_DOWN); 
    selectedItem++;
    if (selectedItem >= menuLength) selectedItem = 0; // Wrap around
    
    // Scroll offset logic to keep selected item visible
    if (selectedItem >= scrollOffset + visibleItems) {
        scrollOffset = selectedItem - visibleItems + 1; 
    } else if (selectedItem == 0) {
        scrollOffset = 0;
    }
    scrollOffset = constrain(scrollOffset, 0, menuLength - visibleItems); 
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    waitForRelease(BTN_SELECT); 
    switch (selectedItem) {
      // Case 0: "READ"
      case 0: runLoop(subghz_read); break;
      // Case 1: "READ RAW"
      case 1: runLoop(subghz_read_raw); break;
      // Case 2: "FREQUENCY ANALYZER"
      case 2: runLoop(subghz_frequency_analyzer); break;
      // Case 3: "JAMMER" (Corrected mapping)
      case 3: runLoop(subghz_jammer); break;
      // Case 4: "SAVED SIGNALS" (Corrected mapping)
      case 4: runLoop(subghz_saved_signals); break;
    }
  }

  // Allow exiting the menu
  if (digitalRead(BTN_BACK) == LOW) {
    waitForRelease(BTN_BACK);
    runLoop(handlemainmenu); 
    return;
  }

  // ===== عرض الشاشة (Display Rendering) =====
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tf); 

  // Draw Menu Items
  for (int i = 0; i < visibleItems; i++) {
    int menuIndex = i + scrollOffset;
    if (menuIndex >= menuLength) break;

    // Y position for text baseline
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

  // ===== شريط التمرير (Scroll Bar) =====
  // Draw a proportional scroll bar for better user feedback
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
