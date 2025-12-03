#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: Assuming external definitions for BTN_*, radio1/2, u8g2, isPressed(), setColor()
// and the debounce utility 'checkButtonPress' from previous fixes.

// --- Global State and Constants ---
// Config
int nrfspesChannel = 40;
int nrfspesPaLevelIndex = 3;     // MAX
int nrfspesDataRateIndex = 2;    // 2MBPS
unsigned long nrfspesJamDuration = 1000; // This constant is unused in the original code, but kept.

int nrfspesSelectedMenu = 0;

const char* menuLabels[] = {"Channel", "PA Level", "Data Rate", "attack"};
const int menuCount = 4;

// Jamming State Machine Variables
static bool specificJammingActive = false;
static bool statusScreenActive = false;
static unsigned long statusScreenStartTime = 0;
const unsigned long STATUS_DISPLAY_DURATION = 1500; // Time (ms) to show "Stopped/Done" message

// Debounce state placeholders (must be defined globally or passed)
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnLeft = HIGH;
static bool lastBtnRight = HIGH;
static bool lastBtnSelect = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long leftPressTime = 0;
static unsigned long rightPressTime = 0;
static unsigned long selectPressTime = 0;

// Placeholder for external functions
extern bool checkButtonPress(int pin, bool &lastState, unsigned long &pressTime); 
extern void runLoop(void (*func)());

// Icon data (must be defined with u8g2_font_...)
static const unsigned char image_bluetooth_not_connected_bits[] U8X8_PROGMEM = {0x40,0x00,0xc1,0x00,0x42,0x01,0x44,0x02,0x48,0x04,0x10,0x04,0x20,0x02,0x40,0x00,0xa0,0x00,0x50,0x01,0x48,0x02,0x44,0x04,0x40,0x0a,0x40,0x11,0x80,0x20,0x00,0x00};
static const unsigned char image_wifi_not_connected_bits[] U8X8_PROGMEM = {0x84,0x0f,0x00,0x68,0x30,0x00,0x10,0xc0,0x00,0xa4,0x0f,0x01,0x42,0x30,0x02,0x91,0x40,0x04,0x08,0x85,0x00,0xc4,0x1a,0x01,0x20,0x24,0x00,0x10,0x4a,0x00,0x80,0x15,0x00,0x40,0x20,0x00,0x00,0x42,0x00,0x00,0x85,0x00,0x00,0x02,0x01,0x00,0x00,0x00};

void updateRadios() {
  // External library types/constants
  extern rf24_pa_dbm_e RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX;
  extern rf24_datarate_e RF24_250KBPS, RF24_1MBPS, RF24_2MBPS;
  extern RF24 radio1, radio2;

  rf24_pa_dbm_e paLevels[] = {RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX};
  rf24_datarate_e rates[] = {RF24_250KBPS, RF24_1MBPS, RF24_2MBPS};

  radio1.setPALevel(paLevels[nrfspesPaLevelIndex]);
  radio2.setPALevel(paLevels[nrfspesPaLevelIndex]);
  radio1.setDataRate(rates[nrfspesDataRateIndex]);
  radio2.setDataRate(rates[nrfspesDataRateIndex]);
}

/**
 * @brief Non-blocking function to draw the active jamming screen.
 */
void drawJammingScreen(int channel, int frequency) {
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);

  // Icons
  u8g2.drawXBMP(4, 5, 14, 16, image_bluetooth_not_connected_bits);
  u8g2.drawXBMP(104, 5, 19, 16, image_wifi_not_connected_bits);

  // Text layers
  char chanStr[5];
  char freqStr[10];
  sprintf(chanStr, "%d", channel);
  sprintf(freqStr, "%d", frequency);
  
  u8g2.setFont(u8g2_font_haxrcorp4089_tr);
  u8g2.drawStr(33, 16, "JAMMING ACTIVE");
  u8g2.drawStr(29, 31, "channel:");
  u8g2.drawStr(40, 46, chanStr);
  u8g2.drawStr(67, 32, freqStr);

  u8g2.sendBuffer();
}

/**
 * @brief Non-blocking jam routine. Runs one pulse and checks for stop.
 */
void nrfspesJam_NonBlocking() {
    extern void setColor(int r, int g, int b);
    extern RF24 radio1, radio2;
    extern bool isPressed(int pin);

    // Generate random data once upon starting jam
    static byte d1[32], d2[32];
    static bool dataInitialized = false;

    if (!dataInitialized) {
        for (int i = 0; i < 32; i++) {
            d1[i] = random(0, 256);
            d2[i] = random(0, 256);
        }
        setColor(225, 0, 0); // Set LED color once
        dataInitialized = true;
    }

    // --- Core Jamming Pulse ---
    radio1.setChannel(nrfspesChannel);
    radio1.stopListening();
    radio1.write(d1, sizeof(d1));

    radio2.setChannel(nrfspesChannel);
    radio2.stopListening();
    radio2.write(d2, sizeof(d2));
    
    // Small delay is necessary for the RF hardware operation (acceptable microsecond blocking)
    delayMicroseconds(100);
    // --- End Jamming Pulse ---

    // Draw status and check for stop
    drawJammingScreen(nrfspesChannel, 2400 + nrfspesChannel);
    
    // Check if user pressed the back button (non-blocking check)
    if (isPressed(BTN_BACK)) {
        specificJammingActive = false; // Stop the jam
        statusScreenActive = true;
        statusScreenStartTime = millis();
        dataInitialized = false; // Reset data flag
    }
}

void drawjammspeschannelMenu() {
  u8g2.clearBuffer();
  u8g2.setFontMode(1); // Opaque font background

  // Box العنوان
  u8g2.setFont(u8g2_font_8x13B_tf);
  u8g2.drawRFrame(10, 5, 108, 18, 3);
  u8g2.setCursor(20, 18);
  u8g2.print("-> ");
  u8g2.print(menuLabels[nrfspesSelectedMenu]);

  // Box القيمة/التفاصيل
  u8g2.drawRFrame(5, 30, 118, 30, 4);
  u8g2.setFont(u8g2_font_7x14B_tf);

  if (nrfspesSelectedMenu == 0) {
    u8g2.setCursor(12, 48);
    u8g2.print("CH: ");
    u8g2.print(nrfspesChannel);
    u8g2.print(" (");
    u8g2.print(2400 + nrfspesChannel);
    u8g2.print(" MHz)");
  } else if (nrfspesSelectedMenu == 1) {
    const char* levels[] = {"MIN", "LOW", "HIGH", "MAX"};
    u8g2.setCursor(35, 48);
    u8g2.print("PA: ");
    u8g2.print(levels[nrfspesPaLevelIndex]);
  } else if (nrfspesSelectedMenu == 2) {
    const char* rates[] = {"250Kbps", "1Mbps", "2Mbps"};
    u8g2.setCursor(30, 48);
    u8g2.print("Rate: ");
    u8g2.print(rates[nrfspesDataRateIndex]);
  } else if (nrfspesSelectedMenu == 3) {
    u8g2.setCursor(20, 46);
    u8g2.print("Press SELECT ");
  }

  u8g2.sendBuffer();
}


void jammspecchannel() {
    extern void setColor(int r, int g, int b);
    extern bool isPressed(int pin);

    // --- STATE 1: Jamming Active ---
    if (specificJammingActive) {
        nrfspesJam_NonBlocking();
        return; 
    }

    // --- STATE 2: Show Status Screen (Stopped/Done) ---
    if (statusScreenActive) {
        unsigned long now = millis();
        
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(0, 30, "Jamming Stopped.");
        u8g2.sendBuffer();
        setColor(0, 0, 0); // Reset LED color
        
        if (now - statusScreenStartTime >= STATUS_DISPLAY_DURATION) {
            statusScreenActive = false; // Transition back to menu
        }
        return;
    }

    // --- STATE 3: Menu Interaction (Configuration) ---

    // Draw menu only when not jamming or showing status
    drawjammspeschannelMenu();

    // Navigation (Non-blocking input)
    if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
      nrfspesSelectedMenu = (nrfspesSelectedMenu - 1 + menuCount) % menuCount;
    }

    if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
      nrfspesSelectedMenu = (nrfspesSelectedMenu + 1) % menuCount;
    }

    // Value Decrement
    if (checkButtonPress(BTN_LEFT, lastBtnLeft, leftPressTime)) {
      if (nrfspesSelectedMenu == 0) nrfspesChannel = max(nrfspesChannel - 1, 0);
      if (nrfspesSelectedMenu == 1) nrfspesPaLevelIndex = max(nrfspesPaLevelIndex - 1, 0);
      if (nrfspesSelectedMenu == 2) nrfspesDataRateIndex = max(nrfspesDataRateIndex - 1, 0);
      updateRadios();
    }

    // Value Increment
    if (checkButtonPress(BTN_RIGHT, lastBtnRight, rightPressTime)) {
      if (nrfspesSelectedMenu == 0) nrfspesChannel = min(nrfspesChannel + 1, 125);
      if (nrfspesSelectedMenu == 1) nrfspesPaLevelIndex = min(nrfspesPaLevelIndex + 1, 3);
      if (nrfspesSelectedMenu == 2) nrfspesDataRateIndex = min(nrfspesDataRateIndex + 1, 2);
      updateRadios();
    }

    // Start Attack
    if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime) && nrfspesSelectedMenu == 3) {
      // Re-configure radios just before the attack, although updateRadios() runs on param changes
      updateRadios(); 
      specificJammingActive = true; // Enter jamming state
    }
}
