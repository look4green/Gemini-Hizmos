#include <Arduino.h>
#include <U8g2lib.h>

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: Replace U8G2_... with your specific display model and configuration.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// Define button pins (Placeholders)
#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_LEFT 7
#define BTN_RIGHT 8
#define BTN_SELECT 9
#define BTN_BACK 10 

// --- 2. GLOBAL STATE VARIABLES ---
int focusDuration = 25; // in minutes
int breakDuration = 5;  // in minutes
bool onBreak = false;
bool isRunning = false;
int currentDuration = 0; // in seconds
unsigned long lastUpdate = 0;
int selectedSetting = 0; // 0 for Focus, 1 for Break

// Static variable for non-blocking debouncing
static unsigned long lastInputTime = 0;
const unsigned long debounceDelay = 200; // 200ms debounce time

// --- 3. UTILITY AND FORWARD DECLARATIONS ---

void runLoop(void (*handler)());
void handlemainmenu(); // Function to return to
void updateTimer(); 
void drawUI();

/**
 * @brief Checks if a button is pressed and handles debouncing.
 * Returns true only on a state change (press), preventing repeated actions.
 * @param pin The button pin to check.
 * @return true if the button is pressed and debounce time has passed.
 */
bool checkButton(int pin) {
  if (digitalRead(pin) == LOW) {
    if (millis() - lastInputTime > debounceDelay) {
      lastInputTime = millis();
      return true;
    }
  }
  return false;
}

/**
 * @brief Waits until the specified button is released.
 * Used for the SELECT and BACK buttons to prevent double-triggering logic.
 * @param pin The button pin to check.
 */
void waitForRelease(uint8_t pin) {
  while (digitalRead(pin) == LOW) {
    delay(5); // Debounce delay while held
  }
}

// Simple placeholder for runLoop
void runLoop(void (*handler)()) {
  handler();
}

/**
 * @brief Resets and sets the current timer duration based on onBreak state.
 */
void updateTimer() {
  currentDuration = (onBreak ? breakDuration : focusDuration) * 60;
  lastUpdate = millis();
}

// --- 4. THE CORRECTED pomdorotimerloop() FUNCTION ---

void pomdorotimerloop(){
  // --- A. Timer Update Logic (Non-blocking 1-second interval) ---
  if (isRunning && millis() - lastUpdate >= 1000) {
    lastUpdate += 1000; // Use += 1000 to maintain clock accuracy
    
    if (currentDuration > 0) {
      currentDuration--;
    } else {
      // Timer finished, swap mode and restart timer
      onBreak = !onBreak;
      updateTimer(); // Sets currentDuration to the new phase duration
      // Optional: Add a buzzer/LED signal here to notify the user
    }
  }

  // --- B. Input and Setting Change Logic (Debounced) ---
  
  // Left/Right: Select Focus or Break setting
  if (checkButton(BTN_LEFT)) selectedSetting = 0;
  if (checkButton(BTN_RIGHT)) selectedSetting = 1;

  // Up/Down: Adjust durations (only when the timer is NOT running)
  if (!isRunning) {
    if (checkButton(BTN_UP)) {
      if (selectedSetting == 0 && focusDuration < 60) focusDuration++;
      if (selectedSetting == 1 && breakDuration < 30) breakDuration++;
      // If duration changes while stopped, update currentDuration to reflect change
      updateTimer(); 
    }

    if (checkButton(BTN_DOWN)) {
      if (selectedSetting == 0 && focusDuration > 1) focusDuration--;
      if (selectedSetting == 1 && breakDuration > 1) breakDuration--;
      // If duration changes while stopped, update currentDuration to reflect change
      updateTimer();
    }
  }

  // SELECT: Start/Stop Timer
  if (checkButton(BTN_SELECT)) {
    // If it's starting and the duration is 0 (first run/reset), initialize it.
    if (!isRunning && currentDuration == 0) {
      updateTimer();
    }
    isRunning = !isRunning;
  }

  // BACK: Reset and Exit Loop
  if (checkButton(BTN_BACK)) {
    // 1. Reset timer state fully
    isRunning = false;
    onBreak = false;
    currentDuration = 0; 
    // 2. Exit this loop and go back to the main menu
    runLoop(handlemainmenu); 
    return; // Exit the function immediately
  }

  // --- C. Display Update ---
  drawUI();
}

// --- 5. CORRECTED drawUI() FUNCTION ---

void drawUI() {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setDrawColor(1);

  // 1. الحالة: FOCUS أو BREAK (Mode String)
  u8g2.setFont(u8g2_font_6x12_tr);
  String modeStr = isRunning ? (onBreak ? "BREAK IN PROGRESS" : "FOCUS IN PROGRESS") : "POMODORO TIMER";
  int w = u8g2.getStrWidth(modeStr.c_str());
  u8g2.setCursor((128 - w) / 2, 10);
  u8g2.print(modeStr);
  
  // 2. إعدادات Focus و Break
  u8g2.setFont(u8g2_font_5x8_tr);
  
  // Highlight the selected setting (Focus or Break) with a background box
  if (selectedSetting == 0) {
      u8g2.drawBox(1, 14, 55, 10); // Focus highlight
      u8g2.setDrawColor(0); // Black text for highlight
  } else {
      u8g2.drawBox(65, 14, 55, 10); // Break highlight
      u8g2.setDrawColor(0);
  }
  
  // Draw Focus setting
  u8g2.setCursor(5, 22);
  u8g2.print("Focus:");
  u8g2.setCursor(40, 22);
  u8g2.print(focusDuration);
  u8g2.print("m");

  // Draw Break setting
  u8g2.setCursor(70, 22);
  u8g2.print("Break:");
  u8g2.setCursor(105, 22);
  u8g2.print(breakDuration);
  u8g2.print("m");
  
  u8g2.setDrawColor(1); // Reset draw color to white text

  // 3. المؤقت (Countdown)
  int minutes = currentDuration / 60;
  int seconds = currentDuration % 60;
  char timeStr[6];
  sprintf(timeStr, "%02d:%02d", minutes, seconds);

  u8g2.drawRFrame(30, 27, 68, 24, 6); // Time frame adjusted for a slightly wider font
  u8g2.setFont(u8g2_font_logisoso20_tr); // Assuming this is a large font
  int tw = u8g2.getStrWidth(timeStr);
  u8g2.setCursor((128 - tw) / 2, 47);
  u8g2.print(timeStr);

  // 4. أزرار التحكم: Start/Stop و Reset/Exit
  u8g2.setFont(u8g2_font_5x8_tr);

  // Start/Stop
  u8g2.drawBox(5, 54, 60, 10);
  u8g2.setDrawColor(0); // Black text on white box
  u8g2.setCursor(10, 62);
  u8g2.print(isRunning ? "[Stop]" : "[Start]");
  u8g2.setDrawColor(1);

  // Reset/Exit
  u8g2.drawBox(68, 54, 55, 10);
  u8g2.setDrawColor(0);
  u8g2.setCursor(76, 62);
  u8g2.print("[ Back ]");
  u8g2.setDrawColor(1);

  u8g2.sendBuffer();
}
