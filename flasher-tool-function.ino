#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: Assuming external definitions for BTN_*, u8g2, SD, File, Update, ESP, 
// and the debounce utility: extern bool checkButtonPress(...)

// Files
#define MAX_FILES 50
String files[MAX_FILES];
int fileCount = 0;
int selectedIndex = 0;
int topIndex = 0;

bool confirmFlash = false;  // وضع تأكيد الفلاش

// --- Non-Blocking Status State ---
static bool statusScreenActive = false;
static String statusMessage = "";
static unsigned long statusStartTime = 0;
const unsigned long STATUS_DISPLAY_DURATION = 2000; // 2 seconds display time

// Debounce state placeholders (must be defined globally or passed)
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnSelect = HIGH;
static bool lastBtnBack = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long selectPressTime = 0;
static unsigned long backPressTime = 0;

// Placeholder for external functions
extern void flashBinary(String path); // Remains blocking for the core flash process
extern void drawMenu();

/**
 * @brief Sets the state for displaying a non-blocking status message.
 */
void displayStatus(const String& msg) {
  statusMessage = msg;
  statusStartTime = millis();
  statusScreenActive = true;
}

/**
 * @brief Draws and manages the status screen based on a timer.
 * @return true if status screen is currently active (blocking menu drawing), false otherwise.
 */
bool displayStatus_NonBlocking() {
  if (!statusScreenActive) return false;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr); 
  
  // Check for timer expiry or manual interrupt (BTN_BACK)
  if (millis() - statusStartTime >= STATUS_DISPLAY_DURATION || 
      (digitalRead(BTN_BACK) == LOW && (millis() - statusStartTime > 100))) {
    statusScreenActive = false;
    drawMenu(); // Immediately redraw the menu after clearing status
    return false;
  }
  
  // Draw current status message
  u8g2.drawStr(0, 30, statusMessage.c_str());
  u8g2.sendBuffer();
  
  return true;
}


void flashBinary(String path) {
  // NOTE: The core Update process must remain blocking, as it involves writing a large stream
  // of data to flash memory, which cannot easily be broken up into non-blocking steps.
  // The surrounding error/success messages are made non-blocking via displayStatus().
  
  File updateBin = SD.open("/" + path);
  if (!updateBin) {
    displayStatus("File not found!");
    return;
  }

  if (updateBin.isDirectory()) {
    displayStatus("Not a file!");
    return;
  }

  size_t updateSize = updateBin.size();

  u8g2.clearBuffer();
  u8g2.drawStr(0, 30, "Flashing...");
  u8g2.sendBuffer();

  if (!Update.begin(updateSize)) {
    displayStatus("Update.begin fail");
    updateBin.close();
    return;
  }

  size_t written = Update.writeStream(updateBin);
  
  updateBin.close(); // Close file after writing stream

  if (written == updateSize && Update.end(true)) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 20, "Update success!");
    u8g2.drawStr(0, 40, "Rebooting...");
    u8g2.sendBuffer();
    delay(2000); // **EXCEPTION: Delay before reboot is often required**
    ESP.restart();
  } else {
    displayStatus("Update failed!");
  }
}

void flasherloop(){
  // --- STATE 1: Displaying Status Message ---
  if (displayStatus_NonBlocking()) {
    return; // Block input and menu drawing while status is showing
  }
  
  // --- STATE 2: Handling Inputs (Non-blocking) ---
  
  if (!confirmFlash) {
    // Menu Navigation
    if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
      if (selectedIndex > 0) {
        selectedIndex--;
        if (selectedIndex < topIndex) topIndex--;
      }
    }

    if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
      if (selectedIndex < fileCount - 1) {
        selectedIndex++;
        // Adjust topIndex to keep selection visible (6 items visible)
        if (selectedIndex >= topIndex + 6) topIndex++; 
      }
    }
  }
  
  // SELECT Button
  if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    if (confirmFlash) {
      // Action: Perform Flash (Blocking)
      String filename = files[selectedIndex];
      confirmFlash = false;
      flashBinary(filename); 
    } else {
      // Action: Request Confirmation
      String filename = files[selectedIndex];
      if (filename.endsWith(".bin")) {
        confirmFlash = true;
      } else {
          displayStatus("Not a .bin file");
      }
    }
  }

  // BACK Button
  if (checkButtonPress(BTN_BACK, lastBtnBack, backPressTime)) {
    if (confirmFlash) {
      // Action: Cancel Confirmation
      confirmFlash = false;
    } else {
      // Action: Exit flasher tool (Assume this is handled by runLoop())
    }
  }
  
  // --- STATE 3: Draw UI (only if not showing status) ---
  drawMenu();
}


void drawMenu() {
  u8g2.clearBuffer();
  u8g2.setDrawColor(1); // Set to white drawing color

  if (confirmFlash) {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 20, "Flash selected file?");
    u8g2.drawStr(0, 35, files[selectedIndex].c_str());
    u8g2.drawStr(0, 50, "SELECT=Yes | BACK=Cancel");
    u8g2.sendBuffer();
    return;
  }

  u8g2.setFont(u8g2_font_6x10_tr);
  
  for (int i = 0; i < 6; i++) {
    int index = topIndex + i;
    if (index >= fileCount) break;

    // 10px height per item
    int y_pos = i * 10; 
    
    if (index == selectedIndex) {
      u8g2.drawBox(0, y_pos, 128, 10);
      u8g2.setDrawColor(0); // Black text on white box
    } else {
      u8g2.setDrawColor(1); // White text on black background
    }

    u8g2.setCursor(2, y_pos + 8); // +8 is based on 10px height
    u8g2.print(files[index]);
  }
  
  u8g2.setDrawColor(1); // Reset draw color
  u8g2.sendBuffer();
}

void readSDfiles() {
  fileCount = 0;
  // External objects assumed: SD, File
  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry || fileCount >= MAX_FILES) break;
    
    // Only process files, check for non-directory
    if (!entry.isDirectory()) {
      files[fileCount++] = String(entry.name());
    }
    entry.close();
  }
}
