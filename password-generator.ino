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

// Debouncing variables
static unsigned long lastInputTime = 0;
const unsigned long debounceDelay = 150; 
const unsigned long holdRepeatDelay = 100; // For hold-to-adjust length

// Input tracking for edge/hold detection
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnLeft = HIGH;
static bool lastBtnRight = HIGH;
static bool lastBtnSelect = HIGH;


// --- 2. GAME STATE VARIABLES ---
bool useNumbers = true;
bool useLetters = true;
bool useSymbols = true;
int passwordLength = 12;
int passwordToggleIndex = 0;
String generatedPassword = "";
bool passwordGenerated = false;
bool inGenMenu = false;
// int genMenuIndex = 0; // Not used in the original simplified logic (only 'Back' exists)


// --- 3. CORE LOGIC (Utility Function) ---

/**
 * @brief Generates a random password based on the given parameters.
 */
String generatePassword(int len, bool useNum, bool useLet, bool useSym) {
  String charset = "";
  if (useNum) charset += "0123456789";
  if (useLet) charset += "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (useSym) charset += "!@#$%^&*()_+=-<>?`~[]{}|\\"; // Extended symbols
    
  if (charset.length() == 0) return "ERR: Select type";

  String pass = "";
  for (int i = 0; i < len; i++) {
    // Ensure randomSeed() has been called in setup()
    char c = charset[random(charset.length())]; 
    pass += c;
  }
  return pass;
}


// --- 4. MAIN HANDLER (Non-blocking) ---

void handlePasswordMaker() {
  const char* options[] = { "Numbers", "Letters", "Symbols", "Length", "Generate" };
  int totalOptions = 5;
  unsigned long now = millis();
  
  // Read current button states (assuming INPUT_PULLUP, LOW when pressed)
  bool btnUp = digitalRead(BTN_UP) == LOW;
  bool btnDown = digitalRead(BTN_DOWN) == LOW;
  bool btnLeft = digitalRead(BTN_LEFT) == LOW;
  bool btnRight = digitalRead(BTN_RIGHT) == LOW;
  bool btnSelect = digitalRead(BTN_SELECT) == LOW;
  
  bool inputTriggered = false; // Flag to track if a debounced action occurred


  // === A. Password View Menu ===
  if (passwordGenerated && inGenMenu) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "Generated:");
    u8g2.setFont(u8g2_font_5x8_tr);
    
    // Display the password, possibly truncated if too long for one line
    if (generatedPassword.length() <= 20) {
        u8g2.drawStr(0, 24, generatedPassword.c_str());
    } else {
        // Display in two parts if needed
        u8g2.drawStr(0, 24, generatedPassword.substring(0, 20).c_str());
        u8g2.drawStr(0, 36, generatedPassword.substring(20).c_str());
    }
    

    // Draw "Back" button
    u8g2.drawBox(40, 50, 48, 12);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(52, 59, "Back");
    u8g2.setDrawColor(1);

    // Handle button press (Non-blocking edge detection)
    if (btnSelect && lastBtnSelect == HIGH) {
        lastInputTime = now;
        passwordGenerated = false;
        inGenMenu = false;
    }

    u8g2.sendBuffer();
    
    // Update button state trackers
    lastBtnSelect = btnSelect;
    return;
  }


  // === B. Main Menu Input Handling (Debounced) ===
  
  // Navigation (UP/DOWN) - Check for press release (HIGH -> LOW)
  if ((btnUp && lastBtnUp == HIGH) || (btnDown && lastBtnDown == HIGH)) {
      if (now - lastInputTime > debounceDelay) {
          lastInputTime = now;
          if (btnUp) {
              passwordToggleIndex = (passwordToggleIndex == 0) ? totalOptions - 1 : passwordToggleIndex - 1;
          } else if (btnDown) {
              passwordToggleIndex = (passwordToggleIndex == totalOptions - 1) ? 0 : passwordToggleIndex + 1;
          }
          inputTriggered = true;
      }
  }

  // Toggle/Adjust Values (LEFT/RIGHT) - Check for initial press OR hold repeat
  bool adjustPressed = btnLeft || btnRight;
  bool adjustAllowed = (adjustPressed && (now - lastInputTime > debounceDelay)) || 
                       (adjustPressed && (now - lastInputTime > holdRepeatDelay));

  if (adjustAllowed) {
      if (passwordToggleIndex >= 0 && passwordToggleIndex <= 2) { // Toggle ON/OFF
          if (passwordToggleIndex == 0) useNumbers = !useNumbers;
          if (passwordToggleIndex == 1) useLetters = !useLetters;
          if (passwordToggleIndex == 2) useSymbols = !useSymbols;
          lastInputTime = now;
          inputTriggered = true;
      } else if (passwordToggleIndex == 3) { // Length adjustment
          if (btnRight && passwordLength < 32) passwordLength++;
          if (btnLeft && passwordLength > 4) passwordLength--;
          lastInputTime = now;
          inputTriggered = true;
      }
  }

  // Generate (SELECT at index 4)
  if (btnSelect && lastBtnSelect == HIGH) {
      if (now - lastInputTime > debounceDelay) {
          if (passwordToggleIndex == 4) {
              generatedPassword = generatePassword(passwordLength, useNumbers, useLetters, useSymbols);
              passwordGenerated = true;
              inGenMenu = true;
              lastInputTime = now;
              inputTriggered = true;
          }
      }
  }

  // Update button state trackers
  lastBtnUp = btnUp;
  lastBtnDown = btnDown;
  lastBtnLeft = btnLeft;
  lastBtnRight = btnRight;
  lastBtnSelect = btnSelect;
  

  // === C. Draw Main Menu ===
  if (!passwordGenerated || inputTriggered) { // Only redraw if state changed or password hasn't been generated
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 10, "Password Generator");
  
      for (int i = 0; i < totalOptions; i++) {
        int y = 20 + i * 10;
        
        // Highlighting selected item
        if (i == passwordToggleIndex) {
          u8g2.drawBox(0, y - 8, 128, 10);
          u8g2.setDrawColor(0); // Black text on white background
        }
    
        u8g2.drawStr(4, y, options[i]);
    
        // Value indicators
        if (i == 0) u8g2.drawStr(90, y, useNumbers ? "ON" : "OFF");
        if (i == 1) u8g2.drawStr(90, y, useLetters ? "ON" : "OFF");
        if (i == 2) u8g2.drawStr(90, y, useSymbols ? "ON" : "OFF");
        if (i == 3) {
          // Length indicator
          u8g2.drawStr(70, y, String(passwordLength).c_str());
          
          // Simple length bar (Adjusted for better visualization)
          u8g2.drawFrame(85, y - 7, 38, 8);
          int barLen = map(passwordLength, 4, 32, 2, 36);
          u8g2.drawBox(86, y - 6, barLen, 6);
        }
    
        u8g2.setDrawColor(1); // Reset to white text
      }
  
      u8g2.sendBuffer();
  }
}
