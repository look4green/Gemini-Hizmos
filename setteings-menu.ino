#include <Arduino.h>
#include <U8g2lib.h>
#include <SD.h> // Required for SD card operations

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---

// Define placeholders for the U8G2 object and button pins
// NOTE: Replace U8G2_... with your specific display model and configuration.
// extern U8G2 u8g2; 
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_SELECT 7
#define BTN_BACK 8 // Assuming a back button exists to exit settings
#define SD_CS 10 // Chip Select pin for SD card

// --- 2. FORWARD DECLARATIONS (Required for compilation) ---

// These are utility functions needed by the menu system
void runLoop(void (*handler)());
void waitForRelease(uint8_t pin);
void drawBoxMessage(const char* line1, const char* line2, const char* line3);

// These are the target functions called when SELECT is pressed
void datausage();
void sdformater();
void restartesp();
void drawnosdcard();
void drawConfirmationUI();
void sdinfo_readStats(); // Assuming this reads SD data
void sdinfo_drawMainScreen(); // Assuming this formats SD data for display
void showsdinfo();
void about();
void checksysdevices();
void handlemainmenu(); // To return to the main menu

// --- 3. UTILITY IMPLEMENTATION ---

// Simple placeholder for runLoop (your system's main state machine)
void runLoop(void (*handler)()) {
  // In a real system, this would change the active function pointer in the main loop.
  // For this example, we just return to the main menu handler immediately.
  // A real implementation would loop until the handler calls runLoop(handlemainmenu).
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

// --- 4. THE CORRECTED handlesettingsmenu() FUNCTION ---

void handlesettingsmenu() {
  const char* menuItems[] = {"show usage", "format sd", "restart", "batt info", "sd info", "about", "check sys devices"};
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3; 

  // Static variables persist between function calls
  static int selectedItem = 0;
  static int scrollOffset = 0;

  // The time-based debounce variables are removed since we use waitForRelease
  // static unsigned long lastInputTime = 0; // Removed

  // ===== التعامل مع الأزرار (Input Handling) =====
  // Check for button press and use waitForRelease for single-press logic

  if (digitalRead(BTN_UP) == LOW) {
    waitForRelease(BTN_UP); 
    selectedItem--;
    if (selectedItem < 0) selectedItem = menuLength - 1; // Wrap around to end
    
    // Adjust scroll offset to keep selectedItem visible (ideally centered or at top)
    if (selectedItem < scrollOffset) {
        scrollOffset = selectedItem; // Scroll up immediately
    } else if (selectedItem == menuLength - 1 && menuLength > visibleItems) {
        scrollOffset = menuLength - visibleItems; // Jump to bottom view on wrap
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
        scrollOffset = 0; // Jump to top view on wrap
    }
    // Re-constrain to prevent going out of bounds
    scrollOffset = constrain(scrollOffset, 0, menuLength - visibleItems); 
  }

  if (digitalRead(BTN_SELECT) == LOW) {
    waitForRelease(BTN_SELECT); 
    switch (selectedItem) {
      case 0: runLoop(datausage); break;
      case 1: // format sd
        if (!SD.begin(SD_CS)) {
          runLoop(drawnosdcard); break;
        }
        // This message is a preliminary warning before entering the formatting routine
        drawBoxMessage("Do you want to", "format SD card?", "Press SELECT to continue");
        runLoop(sdformater); 
        break;
      case 2: // restart
        drawConfirmationUI();
        runLoop(restartesp); 
        break;
      case 3: // batt info (Action was missing, kept placeholder)
        // runLoop(showbatteryinfo);
        break; 
      case 4: // sd info
        if (!SD.begin(SD_CS)) {
          runLoop(drawnosdcard); break;
        }
        sdinfo_readStats();
        sdinfo_drawMainScreen();
        runLoop(showsdinfo);
        break;
      case 5: runLoop(about); break;
      case 6: runLoop(checksysdevices); break;
    }
  }

  // Allow exiting the settings menu with a dedicated back button
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

    // Y position for text baseline (16, 36, 56, etc.)
    int y = i * 20 + 16; 

    if (menuIndex == selectedItem) {
      // Draw highlight box (x, y, w, h, radius)
      u8g2.drawRBox(4, y - 12, 120, 16, 4); 
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
  // Ensure the marker is at least 3 pixels tall
  if (markerHeight < 3) markerHeight = 3; 

  // Position the marker based on the selected item's index
  int markerY = map(selectedItem, 0, menuLength - 1, 0, totalHeight - markerHeight);
  
  // Draw the scroll track and the marker
  u8g2.drawVLine(barX + 1, 0, totalHeight); // Scroll track
  u8g2.drawBox(barX, markerY, 3, markerHeight); // Proportional marker

  u8g2.sendBuffer();
}
