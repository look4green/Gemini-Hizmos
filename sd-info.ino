#include <Arduino.h>
#include <U8g2lib.h>
// Libraries required for SD card operations
#include <SD.h> 
#include <FS.h> 
#include <limits.h> // For UINT64_MAX

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: Replace U8G2_... with your specific display model and configuration.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// Define button pins (Placeholders)
#define BTN_SELECT 7 

// === Global Variables ===
float sdinfo_totalMB = 0, sdinfo_usedMB = 0, sdinfo_freeMB = 0;
int sdinfo_fileCount = 0;
uint64_t sdinfo_largestFile = 0, sdinfo_smallestFile = UINT64_MAX;
bool sdinfo_showDetails = false;

// Debounce state for BTN_SELECT
static bool sdinfo_lastSelectState = HIGH;

// Forward declaration
void sdinfo_drawDetailsScreen();
void sdinfo_drawMainScreen();
void sdinfo_readStats();


// --- 2. INITIALIZATION AND STATS READING ---

// === Init Display ===
void sdinfo_initDisplay() {
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(20, 30, "Initializing SD...");
  u8g2.sendBuffer();

  // Initialize SD Card
  if (!SD.begin(5)) { // Assuming pin 5 for CS (Change this pin if needed)
    u8g2.clearBuffer();
    u8g2.drawStr(10, 30, "SD Card Failed!");
    u8g2.sendBuffer();
    while (1); // Block if SD card fails critical startup
  }

  // Initial read of stats
  sdinfo_readStats(); 
  sdinfo_drawMainScreen(); // Draw the initial screen
}


// === Read SD Stats ===
void sdinfo_readStats() {
    // Check if SD card is mounted/ready before attempting file operations
    if (!SD.cardSize()) return; 

    // Reset stats
    sdinfo_usedMB = 0;
    sdinfo_fileCount = 0;
    sdinfo_largestFile = 0;
    sdinfo_smallestFile = UINT64_MAX;

    File root = SD.open("/");
    if (!root) return; // Handle case where root directory can't be opened

    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
      if (!entry.isDirectory()) {
        uint64_t size = entry.size();
        sdinfo_usedMB += (float)size / (1024.0 * 1024.0);
        sdinfo_fileCount++;
        if (size > sdinfo_largestFile) sdinfo_largestFile = size;
        if (size < sdinfo_smallestFile || sdinfo_smallestFile == UINT64_MAX) sdinfo_smallestFile = size;
      }
      entry.close();
    }
    root.close();

    sdinfo_totalMB = (float)SD.cardSize() / (1024.0 * 1024.0);
    sdinfo_freeMB = sdinfo_totalMB - sdinfo_usedMB;
}


// --- 3. DRAWING FUNCTIONS ---

// === Draw Main Info Screen ===
void sdinfo_drawMainScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(20, 10, "SD Card Info");

  u8g2.drawRBox(0, 14, 128, 14, 4);
  u8g2.setDrawColor(0);
  u8g2.setCursor(4, 24);
  u8g2.printf("Total: %.2f MB", sdinfo_totalMB);
  u8g2.setDrawColor(1);

  u8g2.drawRBox(0, 30, 128, 14, 4);
  u8g2.setDrawColor(0);
  u8g2.setCursor(4, 40);
  u8g2.printf("Used:  %.2f MB", sdinfo_usedMB);
  u8g2.setDrawColor(1);

  u8g2.drawRBox(0, 46, 128, 14, 4);
  u8g2.setDrawColor(0);
  u8g2.setCursor(4, 56);
  u8g2.printf("Free:  %.2f MB", sdinfo_freeMB);
  u8g2.setDrawColor(1);

  u8g2.sendBuffer();
}

// === Draw Detailed Info Screen ===
void sdinfo_drawDetailsScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "Details:");

  u8g2.setCursor(0, 24);
  u8g2.printf("Files: %d", sdinfo_fileCount);

  u8g2.setCursor(0, 36);
  u8g2.printf("Largest: %.2f KB", (float)sdinfo_largestFile / 1024.0);

  u8g2.setCursor(0, 48);
  if (sdinfo_fileCount > 0)
    u8g2.printf("Smallest: %.2f KB", (float)sdinfo_smallestFile / 1024.0);
  else
    u8g2.print("Smallest: N/A");

  u8g2.setCursor(0, 60);
  u8g2.print("Press SELECT to back");

  u8g2.sendBuffer();
}


// --- 4. MAIN LOOP HANDLER (Non-blocking) ---

// === Button Handling (Updated to be non-blocking) ===
void sdinfo_handleButtons() {
    // Use the global static state for edge detection
    static unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 50; 
    
    bool currentSelectState = digitalRead(BTN_SELECT); // HIGH when not pressed

    if (currentSelectState != sdinfo_lastSelectState) {
        lastDebounceTime = millis();
    }
    
    // Check for a press event (LOW signal) after debounce time
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (currentSelectState == LOW && sdinfo_lastSelectState == HIGH) {
            sdinfo_showDetails = !sdinfo_showDetails;
            
            // Redraw the screen based on the new state
            if (sdinfo_showDetails)
                sdinfo_drawDetailsScreen();
            else
                sdinfo_drawMainScreen();
        }
    }

    // Save the current state for the next comparison
    sdinfo_lastSelectState = currentSelectState;
}

void showsdinfo() {
    // The main loop for the SD Info screen
    // It only handles buttons; drawing happens immediately when the state changes
    sdinfo_handleButtons(); 
    // Removed the problematic delay(100);
}

/*
// Example setup/loop structure for a full sketch:
void setup() {
    pinMode(BTN_SELECT, INPUT_PULLUP);
    sdinfo_initDisplay(); // This reads the stats and draws the first screen
}

void loop() {
    showsdinfo();
    // Other functions would run here, like handlesettingsmenu()
}
*/
