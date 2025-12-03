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
