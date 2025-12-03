#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: Assuming external definitions for BTN_*, u8g2, runLoop(), 
// handlesuniversalremotemenu, recvIR, listIRFiles, and the debounce utility:
// extern bool checkButtonPress(int pin, bool &lastState, unsigned long &pressTime);

// Debounce state placeholders (must be defined globally or passed)
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnSelect = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long selectPressTime = 0;

void handleinfraredmenu() {
  const char* menuItems[] = {"UNIVERSAL REMOTE", "LEARN NEW REMOTE", "SAVED SIGNALS"};
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3;

  static int selectedItem = 0;
  static int scrollOffset = 0;

  // --- Non-Blocking Input Handling ---
  
  // Navigation: UP
  if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
    selectedItem--;
    if (selectedItem < 0) selectedItem = menuLength - 1;
    // Recalculate scroll offset
    scrollOffset = constrain(selectedItem - visibleItems + 1, 0, menuLength - visibleItems);
  }

  // Navigation: DOWN
  if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
    selectedItem++;
    if (selectedItem >= menuLength) selectedItem = 0;
    // Recalculate scroll offset
    scrollOffset = constrain(selectedItem - visibleItems + 1, 0, menuLength - visibleItems);
  }

  // Selection: SELECT
  if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    switch (selectedItem) {
      case 0: // UNIVERSAL REMOTE
        runLoop(handlesuniversalremotemenu);
        break;
      case 1: // LEARN NEW REMOTE
        runLoop(recvIR);
        break;
      case 2: // SAVED SIGNALS
        runLoop(listIRFiles);
        break;
      // Cases 3 and 4 are safely ignored since menuLength is 3.
    }
    // When runLoop() is called, the state changes and this function will not be 
    // run again until the new loop exits. If runLoop() is non-blocking and returns 
    // quickly, the menu will redraw next iteration.
  }

  // ===== عرض الشاشة =====
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tf); // Nice clean font

  for (int i = 0; i < visibleItems; i++) {
    int menuIndex = i + scrollOffset;
    // Check bounds against the true menu length, not just visible items
    if (menuIndex >= menuLength) break; 

    int y = i * 20 + 16;

    if (menuIndex == selectedItem) {
      u8g2.drawRBox(4, y - 12, 120, 16, 4); // Rounded highlight
      u8g2.setDrawColor(0); // black text on white box
      u8g2.drawStr(10, y, menuItems[menuIndex]);
      u8g2.setDrawColor(1); // switch back to white text for unselected items
    } else {
      u8g2.drawStr(10, y, menuItems[menuIndex]);
    }
  }

  // ===== شريط التمرير =====
  int barX = 124;
  int barHeight = 64;
  
  // Draw a faint line for the scrollbar track (for better visualization)
  u8g2.drawVLine(barX + 1, 0, barHeight); 

  // Calculate scrollbar position and size (a small box/dot)
  int visibleHeight = barHeight * visibleItems / menuLength;
  int barTop = map(scrollOffset, 0, menuLength - visibleItems, 0, barHeight - visibleHeight);

  // Draw the scroll indicator
  u8g2.drawBox(barX, barTop, 3, visibleHeight); 

  u8g2.sendBuffer();
}

