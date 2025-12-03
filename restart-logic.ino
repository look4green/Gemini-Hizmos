#include <Arduino.h>
#include <U8g2lib.h>
// Assuming you are using an ESP32 or ESP8266
// For standard Arduino, ESP.restart() needs a software watchdog trigger or a custom function.
// For compilation, we define a dummy restart function.
#define ESP_RESTART_AVAILABLE 
#ifndef ESP_RESTART_AVAILABLE
void ESP_restart() { /* Dummy restart function */ } 
#endif

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: Replace U8G2_... with your specific display model.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

#define BTN_SELECT 7
#define BTN_BACK 8 

// --- 2. FORWARD DECLARATIONS and UTILITY FUNCTIONS ---

// Function to return to (e.g., the settings menu)
void runLoop(void (*handler)());
void handlesettingsmenu(); 
void drawConfirmationUI();
void drawCancelledMessage();
void restartDevice();

/**
 * @brief Waits until the specified button is released.
 * @param pin The button pin to check.
 */
void waitForRelease(uint8_t pin) {
  // Assuming buttons are pulled up (HIGH) and LOW when pressed
  while (digitalRead(pin) == LOW) {
    delay(5); 
  }
}

// Simple placeholder for runLoop
void runLoop(void (*handler)()) {
  handler();
}

/**
 * @brief Renders the confirmation message and handles input.
 * This function should be the active state in the main loop until an action is taken.
 */
void restartConfirmationLoop() {
    // 1. Draw the UI every time the function is called
    drawConfirmationUI();
    
    // 2. Check for YES (Restart)
    if (digitalRead(BTN_SELECT) == LOW) {
        waitForRelease(BTN_SELECT); 
        restartDevice();
        // NOTE: restartDevice() will call ESP.restart() and should not return.
        return; 
    }

    // 3. Check for NO (Cancel)
    if (digitalRead(BTN_BACK) == LOW) {
        waitForRelease(BTN_BACK); 
        // Display cancellation message briefly
        drawCancelledMessage();
        u8g2.sendBuffer();
        delay(800); 
        
        // Return control to the previous menu instead of locking the program
        runLoop(handlesettingsmenu); 
        return; 
    }
}

// --- 4. IMPLEMENTATION OF UI AND ACTION FUNCTIONS ---

void drawConfirmationUI() {
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    
    // Frame
    u8g2.drawRFrame(0, 0, 128, 64, 4); 

    // Message Text (Centered)
    u8g2.setFont(u8g2_font_6x13_tr);
    u8g2.drawStr(20, 20, "Are you sure you");
    u8g2.drawStr(20, 34, "want to restart?");

    // Button Area: YES (BTN_SELECT)
    // Highlight YES since it's the action
    u8g2.drawBox(10, 45, 45, 15); 
    u8g2.setDrawColor(0); // Black text
    u8g2.setFont(u8g2_font_6x13_tr);
    u8g2.drawStr(20, 56, "YES");
    u8g2.setDrawColor(1); // Reset to white

    // Button Area: NO (BTN_BACK)
    u8g2.drawFrame(70, 45, 45, 15); 
    u8g2.drawStr(85, 56, "NO"); 

    u8g2.sendBuffer(); // Must call sendBuffer here for the display loop.
}

void drawCancelledMessage() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x13_tr);
    u8g2.drawStr(30, 30, "Cancelled!");
    u8g2.sendBuffer();
}

void restartDevice() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x13_tr);
    u8g2.drawStr(30, 30, "Restarting...");
    u8g2.sendBuffer();
    
    // Delay is fine here as the system is about to reset anyway
    delay(1000); 
    
    // Call the actual restart function
    // For ESP devices:
    // ESP.restart(); 
    // For non-ESP devices, use the appropriate command or the dummy placeholder:
    #ifdef ESP_RESTART_AVAILABLE
    ESP.restart();
    #endif
}
