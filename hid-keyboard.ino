#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: Assuming external definitions for BTN_*, Keyboard (HID library), 
// KEY_*, and the debounce utility:
// extern bool checkButtonPress(int pin, bool &lastState, unsigned long &pressTime);

// --- Debounce state placeholders for BTNs ---
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnLeft = HIGH;
static bool lastBtnRight = HIGH;
static bool lastBtnSelect = HIGH;
static bool lastBtnBack = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long leftPressTime = 0;
static unsigned long rightPressTime = 0;
static unsigned long selectPressTime = 0;
static unsigned long backPressTime = 0;

// ... (Other global variables like hidKeyboard, hidSelectedRow, etc., are unchanged)

// External function assumed to exist
extern void drawHIDKeyboard(); 
extern void handleHIDSpecialKey(int index);

void hidkeyboard() {
  // --- 1. Input Handling (Non-blocking) ---

  // UP Button
  if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
    if (hidInSpecialKeys) {
      // Navigate up within special keys (row-wise movement)
      if (hidSelectedSpecial >= hidSpecialKeysPerRow) {
        hidSelectedSpecial -= hidSpecialKeysPerRow;
      } else {
        // Wrap to main keyboard
        hidInSpecialKeys = false;
        hidSelectedRow = hidRows - 1;
        hidSelectedCol = constrain(hidSelectedCol, 0, hidCols - 1);
        hidScrollOffset = hidRows - hidVisibleRows;
      }
    } else {
      // Navigate up within main keyboard
      if (hidSelectedRow > 0) {
        hidSelectedRow--;
        if (hidSelectedRow < hidScrollOffset) {
          hidScrollOffset--;
        }
      }
    }
  }

  // DOWN Button
  else if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
    if (!hidInSpecialKeys) {
      // Navigate down within main keyboard
      if (hidSelectedRow < hidRows - 1) {
        hidSelectedRow++;
        if (hidSelectedRow >= hidScrollOffset + hidVisibleRows) {
          hidScrollOffset++;
        }
      } else {
        // Wrap to special keys
        hidInSpecialKeys = true;
        hidSelectedSpecial = 0;
      }
    } else {
      // Navigate down within special keys (row-wise movement)
      if (hidSelectedSpecial + hidSpecialKeysPerRow < hidSpecialKeysCount) {
        hidSelectedSpecial += hidSpecialKeysPerRow;
      } else {
        // Wrap to the last row if jumping too far
        // Or keep it simple: stop at the last key
        hidSelectedSpecial = hidSpecialKeysCount - 1;
      }
    }
  }

  // RIGHT Button
  else if (checkButtonPress(BTN_RIGHT, lastBtnRight, rightPressTime)) {
    if (hidInSpecialKeys) {
      // Navigate right within special keys
      if (hidSelectedSpecial < hidSpecialKeysCount - 1) {
        hidSelectedSpecial++;
      }
    } else {
      // Navigate right within main keyboard
      hidSelectedCol = (hidSelectedCol + 1) % hidCols;
    }
  }

  // LEFT Button
  else if (checkButtonPress(BTN_LEFT, lastBtnLeft, leftPressTime)) {
    if (hidInSpecialKeys) {
      // Navigate left within special keys
      if (hidSelectedSpecial > 0) {
        hidSelectedSpecial--;
      }
    } else {
      // Navigate left within main keyboard
      hidSelectedCol = (hidSelectedCol - 1 + hidCols) % hidCols;
    }
  }

  // SELECT Button (Execute Keypress)
  else if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    if (hidInSpecialKeys) {
      // Press a special key
      handleHIDSpecialKey(hidSelectedSpecial);
    } else {
      // Press a character key
      char ch = hidKeyboard[hidSelectedRow][hidSelectedCol];
      Keyboard.print(ch);
    }
  }
  
  // BACK Button (Toggle Keyboard Mode)
  else if (checkButtonPress(BTN_BACK, lastBtnBack, backPressTime)) {
    hidInSpecialKeys = !hidInSpecialKeys;
  }

  // --- 2. Draw the UI ---
  drawHIDKeyboard();
}
