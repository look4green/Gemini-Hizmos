#include <Arduino.h>
#include <U8g2lib.h>

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: Replace U8G2_... with your specific display model and configuration.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// Define button pins (Placeholders)
#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_SELECT 7
#define BTN_BACK 8 // Assuming a back button exists for navigation

// --- 2. FORWARD DECLARATIONS (Required for compilation) ---

// Utility functions
void runLoop(void (*handler)());
void waitForRelease(uint8_t pin);
void handlemainmenu(); // Function to return to
void loading(); // Placeholder for the loading state

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

// --- 4. THE CORRECTED handlesuniversalremotemenu() FUNCTION ---

void handlesuniversalremotemenu() {
  const char* menuItems[] = {"TV", "AC", "PROJECTOR", "CAMERAS"};
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3; 

  static int selectedItem = 0;
  static int scrollOffset = 0;

  // Time-based debounce logic is removed and replaced by waitForRelease.

  // ===== التعامل مع الأزرار (Input Handling using waitForRelease) =====

  // ⬆️ زر UP
  if (digitalRead(BTN_UP) == LOW) {
    waitForRelease(BTN_UP); 
    selectedItem--;
    if (selectedItem < 0) selectedItem = menuLength - 1; // Wrap around
    
    // Scroll offset logic to keep selected item visible
    if (selectedItem < scrollOffset) {
        scrollOffset = selectedItem; 
    } else if (selectedItem == menuLength - 1 && menuLength > visibleItems) {
        scrollOffset = constrain(menuLength - visibleItems, 0, menuLength);
    }
    scrollOffset = constrain(scrollOffset, 0, menuLength - visibleItems); 
  }

  // ⬇️ زر DOWN
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

  // ✔️ زر SELECT
  if (digitalRead(BTN_SELECT) == LOW) {
    waitForRelease(BTN_SELECT); 
    switch (selectedItem) {
      case 0: runLoop(loading); break; // TV
      case 1: runLoop(loading); break; // AC
      case 2: runLoop(loading); break; // PROJECTOR
      case 3: runLoop(loading); break; // CAMERAS
      // No case 4 based on the menuItems array
    }
  }

  // 🔙 زر BACK (Added Exit Logic)
  if (digitalRead(BTN_BACK) == LOW) {
    waitForRelease(BTN_BACK);
    runLoop(handlemainmenu); 
    return;
  }

  // ===== عرض القائمة على الشاشة (Display Rendering) =====
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

  // ===== شريط التمرير الجانبي (Scroll Bar) =====
  int barX = 124;
  int totalHeight = 64; 
  
  // Calculate the size of the marker based on menu length
  int markerHeight = totalHeight / menuLength; 
  if (markerHeight < 3) markerHeight = 3; 

  // Position the marker based on the selected item's index
  int markerY = map(selectedItem, 0, menuLength - 1, 0, totalHeight - markerHeight);
  
  // Draw the scroll track and the marker
  u8g2.drawVLine(barX + 1, 0, totalHeight); // Scroll track
  u8g2.drawBox(barX, markerY, 3, markerHeight); // Proportional marker

  u8g2.sendBuffer();
}
