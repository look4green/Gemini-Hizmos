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
