#include <Arduino.h>
// NOTE: Assuming external definitions for BTN_*, u8g2, Keyboard (HID library), 
// SD (SD library), KEY_*, runLoop, drawnosdcard, and the debounce utility:
// extern bool checkButtonPress(int pin, bool &lastState, unsigned long &pressTime);

// ========== HIDScript: File Variables ==========
String hidFileList[20];
int hidFileCount = 0;
int hidSelectedIndex = 0;
unsigned long hidDefaultDelay = 300;
String hidLastCommand = "";

// Debounce state placeholders (must be defined globally or passed)
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnSelect = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long selectPressTime = 0;

// ========== Execute HID Script (Fixing REPEAT Command) ==========
// NOTE: This function remains blocking due to the nature of script execution, 
// but the REPEAT bug is fixed.
void hidRunScript(const char* path) {
  // External objects assumed: File, SD, Keyboard
  File file = SD.open(path);
  if (!file) {
    hidShowDebug("FAILED TO OPEN FILE");
    return;
  }

  hidShowDebug(String("Running: ") + path);
  
  // --- REPEAT Command Logic Helper ---
  auto executeLastCommand = [&]() {
      if (hidLastCommand.length() == 0) return;

      String line = hidLastCommand;
      String upperLine = line;
      upperLine.toUpperCase();
      
      // Re-evaluate the last command (simplified, no recursion/delay needed here)
      if (upperLine.startsWith("STRING ")) {
          Keyboard.print(line.substring(7));
      } else if (upperLine.startsWith("TYPE ")) {
          char key = line.substring(5).charAt(0);
          Keyboard.press(key);
          delay(100);
          Keyboard.release(key);
      } else if (hidGetSpecialKey(upperLine) > 0) {
          uint8_t key = hidGetSpecialKey(upperLine);
          Keyboard.press(key);
          delay(100);
          Keyboard.release(key);
      } else if (upperLine.startsWith("GUI ") || upperLine.startsWith("WINDOWS ")) {
          uint8_t key = line.substring(line.indexOf(' ') + 1).charAt(0);
          hidPressCombo(true, false, false, false, key);
      } else if (upperLine.startsWith("CTRL ") || upperLine.startsWith("CONTROL ")) {
          uint8_t key = line.substring(line.indexOf(' ') + 1).charAt(0);
          hidPressCombo(false, false, true, false, key);
      } else if (upperLine.startsWith("ALT ")) {
          uint8_t key = line.substring(4).charAt(0);
          hidPressCombo(false, true, false, false, key);
      } else if (upperLine.startsWith("SHIFT ")) {
          uint8_t key = line.substring(6).charAt(0);
          hidPressCombo(false, false, false, true, key);
      }
  };
  // ---------------------------------------------

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    Serial.println("CMD: " + line);
    hidShowDebug("CMD: " + line);
    String upperLine = line;
    upperLine.toUpperCase();

    if (upperLine.startsWith("REM") || upperLine.startsWith("COMMENT")) continue;
    if (upperLine.startsWith("DEFAULT_DELAY ")) {
      hidDefaultDelay = upperLine.substring(14).toInt();
      continue;
    } 
    
    // --- FIXED REPEAT LOGIC ---
    else if (upperLine.startsWith("REPEAT ")) {
      int repeatCount = upperLine.substring(7).toInt();
      if (repeatCount > 0) {
          // The last command already ran once, so we repeat it (count - 1) times.
          for (int i = 0; i < repeatCount - 1; i++) { 
              executeLastCommand();
              delay(hidDefaultDelay); // Add delay between repeats
          }
      }
      // NOTE: This logic assumes 'REPEAT' is only used after a command that performs an action.
    }
    // --- END FIXED REPEAT LOGIC ---
    
    // Commands that set hidLastCommand
    else if (upperLine.startsWith("STRING ")) {
      Keyboard.print(line.substring(7));
      hidLastCommand = line; 
    } else if (upperLine.startsWith("DELAY ")) {
      delay(upperLine.substring(6).toInt());
      hidLastCommand = ""; // DELAY should not be repeated
    } else if (upperLine.startsWith("PRINT ")) {
      hidShowDebug(line.substring(6));
      hidLastCommand = ""; // PRINT should not be repeated
    } else if (upperLine.startsWith("TYPE ")) {
      char key = line.substring(5).charAt(0);
      Keyboard.press(key);
      delay(100);
      Keyboard.release(key);
      hidLastCommand = line;
    } else if (hidGetSpecialKey(upperLine) > 0) {
      uint8_t key = hidGetSpecialKey(upperLine);
      Keyboard.press(key);
      delay(100);
      Keyboard.release(key);
      hidLastCommand = upperLine; // Use upperLine for comparison
    } else if (upperLine.startsWith("GUI ") || upperLine.startsWith("WINDOWS ")) {
      uint8_t key = line.substring(line.indexOf(' ') + 1).charAt(0);
      hidPressCombo(true, false, false, false, key);
      hidLastCommand = line;
    } else if (upperLine.startsWith("CTRL ") || upperLine.startsWith("CONTROL ")) {
      uint8_t key = line.substring(line.indexOf(' ') + 1).charAt(0);
      hidPressCombo(false, false, true, false, key);
      hidLastCommand = line;
    } else if (upperLine.startsWith("ALT ")) {
      uint8_t key = line.substring(4).charAt(0);
      hidPressCombo(false, true, false, false, key);
      hidLastCommand = line;
    } else if (upperLine.startsWith("SHIFT ")) {
      uint8_t key = line.substring(6).charAt(0);
      hidPressCombo(false, false, false, true, key);
      hidLastCommand = line;
    }

    delay(hidDefaultDelay);
  }

  file.close();
  hidShowDebug("Done.");
  delay(1000);
}

void hidscriptmenu() {
  // Draw the menu every loop iteration
  hidDrawMenu();

  if (hidFileCount == 0) {
      // Show error/scan screen if no files are found
      hidShowDebug("No .duck files found.");
      return;
  }
  
  // UP Navigation (Non-blocking)
  if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
    hidSelectedIndex = (hidSelectedIndex - 1 + hidFileCount) % hidFileCount;
  } 
  
  // DOWN Navigation (Non-blocking)
  else if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
    hidSelectedIndex = (hidSelectedIndex + 1) % hidFileCount;
  } 
  
  // SELECT (Action) (Non-blocking)
  else if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    String path = "/" + hidFileList[hidSelectedIndex];
    
    // NOTE: This call is blocking, freezing the device until the script is finished.
    // For HID scripts, blocking is often intentional to ensure command timing.
    hidRunScript(path.c_str());
    
    // After running the blocking script, the menu must be redrawn in the next loop.
  }
}

