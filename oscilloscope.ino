#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h> // For sin, PI
#include <string.h> // For strcmp, memcpy
#include <stdlib.h> // For dtostrf (on some platforms)

// --- GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// Define button pins (Placeholders - must be defined externally)
#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_LEFT 7
#define BTN_RIGHT 8
#define BTN_SELECT 9
#define BTN_BACK 10
#define ANALOG_PIN A0 // Placeholder for the analog input pin

// Placeholder for external function call
extern void runLoop(void (*func)());

// Non-Blocking Debounce Utility
static unsigned long lastInputTime = 0;
const unsigned long debounceDelay = 150; 
static bool btnState[5]; // UP, DOWN, LEFT, RIGHT, SELECT

/**
 * @brief Checks if a button (pin) was pressed since the last time check, 
 * handling debouncing non-blockingly.
 * @param pin The button pin to check.
 * @param lastState Pointer to the static state variable tracking the button's last read state.
 * @return true if a clean press (HIGH to LOW transition) is detected.
 */
bool checkButtonPress(int pin, bool &lastState) {
    bool currentState = (digitalRead(pin) == LOW);
    bool pressed = false;
    unsigned long now = millis();

    if (currentState != lastState) {
        lastInputTime = now;
    }
    
    // Detect press edge (HIGH -> LOW) after debounce time
    if ((now - lastInputTime > debounceDelay) && currentState && !lastState) {
        pressed = true;
    }
    
    lastState = currentState;
    return pressed;
}
