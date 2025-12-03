#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: RF24 library must be included externally, and radio1/radio2 defined.
// #include <RF24.h> 

// Constants
const uint8_t SCREEN_WIDTH = 128;
const uint8_t SCREEN_HEIGHT = 64;
const uint8_t GRAPH_HEIGHT = 54;
const uint8_t TOTAL_CHANNELS = 126;
const uint8_t SAMPLES = 4;

// Data arrays
uint8_t strength1[TOTAL_CHANNELS] = {0};
uint8_t strength2[TOTAL_CHANNELS] = {0};
uint16_t hits1[TOTAL_CHANNELS] = {0};
uint16_t hits2[TOTAL_CHANNELS] = {0};
uint8_t mostActive1 = 0;
uint8_t mostActive2 = 0;

// Non-Blocking State Variables
static unsigned long lastScanTime = 0;
const unsigned long scanInterval = 120; // Target scan interval (was a delay)
static bool scanInProgress = false; // Flag to indicate if we are mid-scan
static uint8_t currentScanChannel = 0; // Tracks progress through the 126 channels

// Placeholder for external objects/functions
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2; 
// Assuming radio objects have startListening(), setChannel(), and testRPD()
// extern RF24 radio1, radio2; 
void drawnrfGraph();

/**
 * @brief Performs a full scan of all 126 channels.
 * NOTE: Due to the nature of testRPD(), this block remains momentarily blocking
 * for the total scan time (126 channels * 2 radios * 4 samples * 130us/sample ≈ 131ms).
 * This is acceptable as it runs only when the scan interval is met.
 */
void scanAll_Blocking() {
  for (uint8_t ch = 0; ch < TOTAL_CHANNELS; ch++) {
    uint8_t r1 = 0, r2 = 0;

    // Sample loop (still contains delays, unavoidable for hardware timing)
    for (uint8_t i = 0; i < SAMPLES; i++) {
      // --- Radio 1 ---
      // Setting channel takes time. TestRPD() requires a brief listening period.
      radio1.setChannel(ch);
      delayMicroseconds(130); 
      if (radio1.testRPD()) {
        r1++;
        hits1[ch]++;
      }

      // --- Radio 2 ---
      radio2.setChannel(ch);
      delayMicroseconds(130);
      if (radio2.testRPD()) {
        r2++;
        hits2[ch]++;
      }
    }

    // Calculate instantaneous strength for graph display
    strength1[ch] = map(r1, 0, SAMPLES, 0, GRAPH_HEIGHT);
    strength2[ch] = map(r2, 0, SAMPLES, 0, GRAPH_HEIGHT);
  }

  // Calculate the most active channel *after* the full scan
  uint16_t max1 = 0, max2 = 0;
  for (uint8_t ch = 0; ch < TOTAL_CHANNELS; ch++) {
    if (hits1[ch] > max1) {
      max1 = hits1[ch];
      mostActive1 = ch;
    }
    if (hits2[ch] > max2) {
      max2 = hits2[ch];
      mostActive2 = ch;
    }
  }
}

void drawnrfGraph() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr); // Set small font for header and labels

  // Header (Current Max Activity)
  char label[32];
  sprintf(label, "R1 Ch%d:H%d | R2 Ch%d:H%d", mostActive1, hits1[mostActive1], mostActive2, hits2[mostActive2]);
  u8g2.drawStr(0, 8, label);

  float channelStep = (float)(TOTAL_CHANNELS - 1) / (SCREEN_WIDTH - 1); 

  int prevY2 = -1;

  for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {
    // Map screen pixel x to a channel index
    uint8_t ch = round(x * channelStep);
    if (ch >= TOTAL_CHANNELS) continue;

    uint8_t h1 = strength1[ch];
    uint8_t h2 = strength2[ch];

    // Simple averaging for smoothing, ensuring boundary checks
    uint8_t smoothed_h1 = h1;
    uint8_t smoothed_h2 = h2;
    
    // Average with neighbors for smoothing (original smoothing logic fixed)
    int count = 1;
    if (ch > 0) {
        smoothed_h1 += strength1[ch - 1];
        smoothed_h2 += strength2[ch - 1];
        count++;
    }
    if (ch < TOTAL_CHANNELS - 1) {
        smoothed_h1 += strength1[ch + 1];
        smoothed_h2 += strength2[ch + 1];
        count++;
    }

    smoothed_h1 /= count;
    smoothed_h2 /= count;


    uint8_t y1 = GRAPH_HEIGHT - smoothed_h1;
    uint8_t y2 = GRAPH_HEIGHT - smoothed_h2;

    // Radio 1 - filled vertical bar
    u8g2.drawVLine(x, y1 + 10, smoothed_h1); // Added 10px offset for header

    // Radio 2 - mountain graph (line plot)
    if (prevY2 >= 0) {
      u8g2.drawLine(x - 1, prevY2, x, y2 + 10); // Added 10px offset for header
    }
    prevY2 = y2 + 10;
  }

  // Base line
  u8g2.drawLine(0, GRAPH_HEIGHT + 10, SCREEN_WIDTH, GRAPH_HEIGHT + 10); // Added 10px offset

  // Channel labels (0, 25, 50, 75, 100, 125)
  for (uint8_t ch_val = 0; ch_val <= 125; ch_val += 25) {
    uint8_t x = round((float)ch_val / 125.0 * (SCREEN_WIDTH - 1));
    sprintf(label, "%d", ch_val);
    u8g2.drawStr(x, SCREEN_HEIGHT, label);
  }

  u8g2.sendBuffer();
}

void nrfscanner() {
    unsigned long now = millis();
    
    // Check if enough time has passed since the last full scan
    if (now - lastScanTime >= scanInterval) {
        // Perform the full scan (this is a momentary blocking operation)
        scanAll_Blocking();
        lastScanTime = now;
        
        // Draw the graph immediately after a scan completes
        drawnrfGraph();
    }
    
    // If we're not scanning or drawing, the loop returns quickly, 
    // keeping the system responsive.
}

