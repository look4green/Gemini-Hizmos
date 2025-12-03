#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h> // Required for WiFi.scanNetworks() and WiFi.mode()

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: Replace U8G2_... with your specific display model and configuration.
// extern U8G2 u8g2; 
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_SELECT 7
#define BTN_BACK 8 

// Wi-Fi Status Globals
int wifi_networkCount = 0;

// --- 2. FORWARD DECLARATIONS (Required for compilation) ---

// Utility functions
void runLoop(void (*handler)());
void waitForRelease(uint8_t pin);
void handlemainmenu(); // Function to return to

// Target functions (your Wi-Fi actions)
void scanningwifi(); // The loop that shows scan animation and results
void setupSnifferGraph();
void updateSnifferGraph();
void loading();


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

// --- 4. THE CORRECTED handlewifimenu() FUNCTION ---

void handlewifimenu() {
  const char* menuItems[] = {"SCAN WIFI", "PACKET ANALYZER", "BEACON", "CAPTIVE PORTAL", "DEAUTH DETECTOR"};
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3; 

  static int selectedItem = 0;
  static int scrollOffset = 0;

  // ===== التعامل مع الأزرار (Input Handling using waitForRelease) =====

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
      // Case 0: SCAN WIFI (Initialization and state change)
      case 0:
        // *** CLEANED UP LOGIC ***
        // 1. Put Wi-Fi initialization here (only runs ONCE on selection)
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100); 
        // 2. Start the scan process (blocking or non-blocking depending on implementation)
        wifi_networkCount = WiFi.scanNetworks(false, true); // Non-blocking: false, Hidden: true

        // 3. Change the main loop handler to the scanning UI
        runLoop(scanningwifi);
        break;
        
      // Case 1: PACKET ANALYZER
      case 1:
        setupSnifferGraph();
        runLoop(updateSnifferGraph);
        break;
        
      // Case 2-4: Other features (use 'loading' as temporary placeholder)
      case 2: runLoop(loading); break;
      case 3: runLoop(loading); break;
      case 4: runLoop(loading); break;
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
  int totalHeight = 64; 
  
  int markerHeight = totalHeight / menuLength; 
  if (markerHeight < 3) markerHeight = 3; 

  int markerY = map(selectedItem, 0, menuLength - 1, 0, totalHeight - markerHeight);
  
  u8g2.drawVLine(barX + 1, 0, totalHeight); 
  u8g2.drawBox(barX, markerY, 3, markerHeight); 

  u8g2.sendBuffer();
}

// --- 5. PLACEHOLDER FOR SCANNING UI (Moved from Case 0) ---
// This function would run continuously when the user selects "SCAN WIFI"
void scanningwifi() {
    // Icons provided in the original case 0 code
    static const unsigned char image_file_search_bits[] U8X8_PROGMEM = {0x80,0x0f,0x40,0x10,0x20,0x20,0x10,0x40,0x10,0x40,0x10,0x50,0x10,0x50,0x10,0x48,0x20,0x26,0x50,0x10,0xa8,0x0f,0x14,0x00,0x0a,0x00,0x05,0x00,0x03,0x00,0x00,0x00};
    static const unsigned char image_wifi_50_bits[] U8X8_PROGMEM = {0x80,0x0f,0x00,0x60,0x30,0x00,0x18,0xc0,0x00,0x84,0x0f,0x01,0x62,0x30,0x02,0x11,0x40,0x04,0x0a,0x87,0x02,0xc4,0x1f,0x01,0xe8,0xb8,0x00,0x70,0x77,0x00,0xa0,0x2f,0x00,0xc0,0x1d,0x00,0x80,0x0a,0x00,0x00,0x07,0x00,0x00,0x02,0x00,0x00,0x00,0x00};

    // This loop should check if the scan is complete (wifi_networkCount >= 0)
    // and display the animation until it is.

    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);

    // file_search
    u8g2.drawXBMP(10, 21, 15, 16, image_file_search_bits);

    // wifi_50
    u8g2.drawXBMP(99, 20, 19, 16, image_wifi_50_bits);

    // Text
    u8g2.setFont(u8g2_font_t0_13_tr);
    u8g2.drawStr(32, 15, "scanning");
    u8g2.drawStr(44, 32, "wifi ");
    u8g2.drawStr(33, 49, "networks");

    // Add animation or progress indicator here if needed
    // Example: Add pulsing light animation or dots for loading

    u8g2.sendBuffer();

    // Placeholder logic to return to the menu once the scan completes
    if (wifi_networkCount >= 0) {
        // Here you would typically process the results, then return to a results display function:
        // runLoop(displayWifiResults);
        // For simplicity, we return to the main menu after a short pause:
        delay(1000); 
        runLoop(handlemainmenu); 
        return;
    }
}
