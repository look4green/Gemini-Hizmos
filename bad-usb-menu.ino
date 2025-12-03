// Utility function to draw the screen at a given progress percentage
// All XBMP data (image_download_bits, image_EviSmile1_bits) must be defined globally.
void drawRunningScreen(String taskName, float progressPercent) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 15, "Running:");
  u8g2.drawStr(0, 30, taskName.c_str());
  u8g2.drawFrame(0, 45, 128, 10); // الإطار

  // (Icon drawing removed here for code clarity, assuming XBMPs are globally accessible)

  // Calculate progress bar width (max 126 pixels for 128px screen frame)
  int barWidth = (int)(126.0 * progressPercent);
  u8g2.drawBox(1, 46, barWidth, 8); // تقدم الشريط

  u8g2.sendBuffer();
}

// --- Global State & Debounce Placeholders (Must be defined globally) ---
// static int currentScriptId = -1; // New global state to track the running script
// static bool lastBtnUp = HIGH;
// static bool lastBtnDown = HIGH;
// static bool lastBtnSelect = HIGH;
// static unsigned long upPressTime = 0;
// static unsigned long downPressTime = 0;
// static unsigned long selectPressTime = 0; 

// --- Assumed external functions:
// extern void hidInit();
// extern void hidkeyboard();
// extern void hidscriptmenu();
// extern void hid_runner(); // New function required for asynchronous script execution

void handlebadusbmenu() {
  const char* menuItems[] = {
    "DEMO", "KEYBOARD", "HID SCRIPT", "Open Notepad", "Open CMD", "Show IP", "Shutdown", "RickRoll", 
    "Create Admin", "Disable Defender", "Open YouTube", "Lock PC", "Fake Update", "Endless Notepad",
    "Fake BSOD", "Flip Screen", "Matrix Effect", "I'm Watching U", "Open Google", "Open telegram",
    "Play Alarm Sound", "Endless CMD", "Type Gibberish", "Spam CAPSLOCK", "Open Calc", "Auto 'Hacked!'",
    "Turn Off Monitor", "Open RegEdit", "Kill Explorer", "Flash Screen", "Rename Desktop", 
    "Toggle WiFi", "Auto Screenshot", "Spam Emojis", "Open Ctrl Panel", "Troll Wallpaper", 
    "Open MS Paint", "Tab Switcher"
  };
  const int menuLength = sizeof(menuItems) / sizeof(menuItems[0]);
  const int visibleItems = 3;

  static int selectedItem = 0;
  static int scrollOffset = 0;

  // ===== التعامل مع الأزرار (Non-blocking input) =====

  // UP Navigation
  if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
    selectedItem--;
    if (selectedItem < 0) selectedItem = menuLength - 1;
    if (selectedItem < scrollOffset) scrollOffset = selectedItem;
    if (selectedItem == menuLength - 1) scrollOffset = menuLength - visibleItems;
    scrollOffset = constrain(scrollOffset, 0, menuLength - visibleItems);
  }

  // DOWN Navigation
  if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
    selectedItem++;
    if (selectedItem >= menuLength) selectedItem = 0;
    if (selectedItem >= scrollOffset + visibleItems) scrollOffset++;
    if (selectedItem == 0) scrollOffset = 0;
    scrollOffset = constrain(scrollOffset, 0, menuLength - visibleItems);
  }

  // SELECT Action
  if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    currentScriptId = selectedItem; 

    switch (currentScriptId) {
      case 1: // KEYBOARD
        runLoop(hidkeyboard);
        return;
      case 2: // HID SCRIPT
        hidInit();
        runLoop(hidscriptmenu);
        return;
      default: // All other cases are blocking scripts, now handled asynchronously
        hidInit();
        runLoop(hid_runner); // CRITICAL FIX: Enter the asynchronous runner loop
        return;
    }
  }
  
  // ===== عرض الشاشة (Drawing UI) =====
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

  // ===== شريط التمرير (Scroll Bar) =====
  int barX = 124;
  int scrollAreaHeight = 60; 
  int spacing = scrollAreaHeight / menuLength; 

  for (int i = 0; i < menuLength; i++) {
    int dotY = i * spacing + 5;
    if (i == selectedItem) {
      u8g2.drawBox(barX, dotY - 3, 3, 6);
    } else {
      u8g2.drawPixel(barX + 1, dotY);
    }
  }

  u8g2.sendBuffer();
}

// NOTE: runCommand remains the same, but it is now called inside hid_runner.
void runCommand(const char* command) {
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('r');
  delay(100); // This delay is acceptable here because it is a momentary HID sequence
  Keyboard.releaseAll();
  delay(300); // This delay is acceptable here because it is a momentary HID sequence
  Keyboard.print(command);
  Keyboard.write(KEY_RETURN);
}

void hid_runner() {
    // These static variables maintain the script's progress across loop iterations
    static int currentStep = 0;
    static unsigned long nextActionTime = 0;
    static int scriptId = -1;
    
    // Initialization: only runs once when the script starts
    if (scriptId == -1) {
        scriptId = currentScriptId; // Copy script ID from the global state
        currentStep = 0;
        // Optionally draw the running screen once
    }

    // Non-blocking timer: exit if it's not time for the next action
    if (millis() < nextActionTime) {
        // Draw the running screen, passing the progress percentage
        // drawRunningScreen(menuItems[scriptId], (float)currentStep / totalSteps[scriptId]);
        return; 
    }

    // --- State Machine ---
    switch (scriptId) {
        case 0: // DEMO
            if (currentStep == 0) {
                // Example of replacing a delay:
                // Old: runCommand("notepad"); delay(1500);
                
                // New:
                runCommand("notepad"); 
                nextActionTime = millis() + 1500;
                currentStep++;
            } else if (currentStep == 1) {
                // Execute next action (e.g., printing the banner)
                Keyboard.println("...");
                nextActionTime = millis() + 200;
                currentStep++;
            }
            // ... continue until the script is finished ...
            break;

        case 6: // Shutdown (Short script, can be one step)
            if (currentStep == 0) {
                runCommand("shutdown /s /t 0");
                nextActionTime = millis() + 500; // Wait for command to register
                currentStep = 99; // Mark as finished
            }
            break;
            
        // Add cases 3 through 37 here, rewriting each step to use nextActionTime
    }
    
    // --- Termination: Exit the runner and return to the main menu ---
    if (currentStep == 99) {
        scriptId = -1; // Reset state
        exitLoop(); // Assumed function to return to the previous loop (handlebadusbmenu)
    }
}

