#include <Arduino.h>
#include <U8g2lib.h>

// --- GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: u8g2, BTN_*, RF24_PA_MAX, RF24_2MBPS, RF24_... are external.
// Assuming the following global objects exist:
// extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2; 
// extern RF24 radio1, radio2; 
// extern void runLoop(void (*func)());
// extern void nrfscanner(), nrfjammer(), jammspecchannel();

#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_SELECT 9

// Debounce constants
const unsigned long debounceDelay = 150; 

// Input tracking for edge detection
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnSelect = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long selectPressTime = 0;

/**
 * @brief Checks if a button (pin) was pressed since the last time check, 
 * handling debouncing non-blockingly using edge detection.
 * @param pin The button pin to check.
 * @param lastState Reference to the static state variable tracking the button's last read state.
 * @param pressTime Reference to the static variable tracking the time the button state changed.
 * @return true if a clean press (HIGH to LOW transition) is detected.
 */
bool checkButtonPress(int pin, bool &lastState, unsigned long &pressTime) {
    bool currentState = (digitalRead(pin) == LOW);
    bool pressed = false;
    unsigned long now = millis();

    // 1. Detect state change
    if (currentState != lastState) {
        pressTime = now;
    }
    
    // 2. Check for debounced edge detection (HIGH -> LOW press)
    if ((now - pressTime > debounceDelay) && currentState && !lastState) {
        pressed = true;
    }
    
    // 3. Update last state for the next loop iteration
    lastState = currentState;
    return pressed;
}

// --- Placeholder/External Functions ---
namespace nrf {
    int currentPage = 0;
}
extern void drawMenuPage(int page);
extern void updateRadios();
extern void drawjammspeschannelMenu();

void handlenrftoolsmenu() {
  const char* menuItems[] = { "analyzer", "hijack nrf device" ,"2.4 ghz jammer","jamm spes channel"};
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3;

  static int selectedItem = 0;
  static int scrollOffset = 0;

  // --- Non-Blocking Input Handling ---
  
  // Navigation: UP
  if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
    selectedItem--;
    if (selectedItem < 0) selectedItem = menuLength - 1;
    scrollOffset = constrain(selectedItem - visibleItems + 1, 0, menuLength - visibleItems);
  }

  // Navigation: DOWN
  if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
    selectedItem++;
    if (selectedItem >= menuLength) selectedItem = 0;
    scrollOffset = constrain(selectedItem - visibleItems + 1, 0, menuLength - visibleItems);
  }

  // Selection: SELECT
  if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    switch (selectedItem) {
      case 0: // analyzer
        // NOTE: These settings are better placed in the nrfscanner setup/init function
        // radio1.setPALevel(RF24_PA_MAX);
        // radio1.setDataRate(RF24_2MBPS);
        // radio1.setAutoAck(false);
        // radio2.setPALevel(RF24_PA_MAX);
        // radio2.setDataRate(RF24_2MBPS);
        // radio2.setAutoAck(false);
        // radio1.startListening();
        // radio2.startListening();
        runLoop(nrfscanner);
        break;
      
      case 1: // hijack nrf device
        // runLoop(nrfhijack); // Placeholder for actual implementation
        break;
      
      case 2: // 2.4 ghz jammer
        // radio configuration should be inside nrfjammer init
        // drawMenuPage(nrf::currentPage); // This looks out of place here
        runLoop(nrfjammer);
        break;

        case 3: // jamm spes channel
        // updateRadios(); // Should be inside jammspecchannel init
        // drawjammspeschannelMenu(); // Should be inside jammspecchannel init
        runLoop(jammspecchannel);
        break;
    }
  }

  // ===== عرض الشاشة =====
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

  // ===== شريط التمرير =====
  int barX = 124;
  int barHeight = 64;
  // Draw a faint line for the scrollbar track
  u8g2.drawVLine(barX + 1, 0, barHeight); 

  // Calculate scrollbar position and size (a small box/dot)
  int visibleHeight = barHeight * visibleItems / menuLength;
  int barTop = map(scrollOffset, 0, menuLength - visibleItems, 0, barHeight - visibleHeight);

  // Draw the scroll indicator (Box for better visibility)
  u8g2.drawBox(barX, barTop, 3, visibleHeight); 
  u8g2.sendBuffer();
}
