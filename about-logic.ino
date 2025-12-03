#include <Arduino.h>
#include <U8g2lib.h>

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// Define placeholders for the U8G2 object and button pins
// extern U8G2 u8g2; 
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_SELECT 7
#define BTN_BACK 8 

// --- 2. GLOBAL MENU DATA ---
// Messages defined in the original code
const char* lines[] = {
  "Project by Hiktron",
  "Instagram: hiktron",
  "YouTube: hiktron",
  "Support us",
  "For more open",
  "source projects",
  "cyber boy",
};

const int totalLines = sizeof(lines) / sizeof(lines[0]);
const int visibleLines = 3;
// topLine remains static in the about() function scope.

// --- 3. FORWARD DECLARATIONS and UTILITY FUNCTIONS ---

// Function to return to (e.g., the settings menu)
void runLoop(void (*handler)());
void handlesettingsmenu();

// Simple placeholder for runLoop
void runLoop(void (*handler)()) {
  handler();
}

/**
 * @brief Draws the proportional scroll bar on the right side.
 * Assumes u8g2.sendBuffer() is called afterward.
 * @param topLine The index of the first visible line.
 */
void drawScrollBar(int topLine) {
  int barHeight = 64;
  
  // Calculate handle height proportionally
  int handleHeight = (barHeight * visibleLines) / totalLines;
  if (handleHeight < 4) handleHeight = 4; // Ensure minimum height

  // Calculate handle position (proportional mapping)
  int scrollRange = totalLines - visibleLines;
  int positionRange = barHeight - handleHeight;
  
  int handleY = 0;
  if (scrollRange > 0) {
      handleY = map(topLine, 0, scrollRange, 0, positionRange);
  }

  u8g2.drawFrame(123, 0, 4, barHeight);     // Scrollbar track
  u8g2.drawBox(124, handleY, 2, handleHeight);   // Scrollbar handle
}


// --- 4. THE CORRECTED about() FUNCTION ---

void about() {
  // Static variables persist between function calls
  static int topLine = 0;
  static unsigned long lastInputTime = 0;
  const long debounceDelay = 200; // Time in milliseconds for debounce

  // --- Input Handling ---
  if (millis() - lastInputTime > debounceDelay) {
    
    // ⬇️ زر DOWN
    if (digitalRead(BTN_DOWN) == LOW) {
      if (topLine < totalLines - visibleLines) {
        topLine++;
      }
      lastInputTime = millis();
    }

    // ⬆️ زر UP
    if (digitalRead(BTN_UP) == LOW) {
      if (topLine > 0) {
        topLine--;
      }
      lastInputTime = millis();
    }
    
    // Return to Settings Menu
    if (digitalRead(BTN_SELECT) == LOW || digitalRead(BTN_BACK) == LOW) {
      // Reset scroll position for next time the screen is opened
      topLine = 0; 
      runLoop(handlesettingsmenu);
      return; // Exit the function to prevent drawing the screen below
    }
  }

  // --- Draw screen ---
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setFontMode(1); // Transparent background

  // Title
  u8g2.setFont(u8g2_font_7x14_tf);
  u8g2.drawStr(10, 8, "About Project");
  u8g2.drawHLine(0, 10, 128);

  // Use a smaller font for the content
  u8g2.setFont(u8g2_font_6x12_tf);

  // Draw the 3 visible lines
  for (int i = 0; i < visibleLines; i++) {
    int index = topLine + i;
    if (index < totalLines) {
      // Start drawing below the title (approx Y=18)
      u8g2.setCursor(5, 25 + i * 12); 
      u8g2.print(lines[index]);
    }
  }

  // Draw scroll indicator
  drawScrollBar(topLine); 

  u8g2.sendBuffer();
}
