#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
// Required for ESP32 Wi-Fi specific functions
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_private/wifi.h> 

// --- 1. GLOBAL CONTEXT & CONFIGURATION ---

// Define display object (Placeholder)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// إعدادات الجراف
#define GRAPH_WIDTH     128
#define GRAPH_HEIGHT    44
#define GRAPH_TOP       12 // Adjusted for score/info line
#define MAX_POINTS      128
#define SPIKE_THRESHOLD 30 // Packets per 200ms interval (150 Pkts/s)

// Define critical section lock for shared data
portMUX_TYPE snifferMux = portMUX_INITIALIZER_UNLOCKED;


// تعريف struct قبل استخدامه
struct SnifferGraph {
  uint8_t graphData[MAX_POINTS] = {0}; // Initialize to zero
  uint8_t currentChannel = 1;
  volatile uint16_t packetCounter = 0; // MUST be volatile
  unsigned long lastChannelSwitch = 0;
  unsigned long lastUpdate = 0;
};

// تعريف كائن الحالة
SnifferGraph sniffer;

// --- 2. SNIFFER CORE FUNCTIONS ---

// رد نداء للباكتات المستلمة
void IRAM_ATTR snifferCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  // Check only for required types
  if (type == WIFI_PKT_MGMT || type == WIFI_PKT_DATA || type == WIFI_PKT_CTRL) {
    // CRITICAL SECTION: Protect shared variable update
    portENTER_CRITICAL_ISR(&snifferMux);
    sniffer.packetCounter++;
    portEXIT_CRITICAL_ISR(&snifferMux);
  }
}

void initDisplay() {
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 10, "Starting analyzer...");
  u8g2.sendBuffer();
}

void initSniffer(struct SnifferGraph &g) {
  // 1. De-initialize WiFi (Robust cleanup)
  WiFi.disconnect(true, true);
  esp_wifi_stop();
  // delay(200); // Removed blocking delay
  esp_wifi_deinit();

  // 2. Initialize WiFi in NULL mode (Required for promiscuous mode)
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();

  // 3. Set promiscuous mode and callback
  esp_wifi_set_channel(g.currentChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_rx_cb(snifferCallback);
  esp_wifi_set_promiscuous(true);
}

void switchChannel(struct SnifferGraph &g) {
  g.currentChannel++;
  if (g.currentChannel > 13) g.currentChannel = 1;
  esp_wifi_set_channel(g.currentChannel, WIFI_SECOND_CHAN_NONE);
}

void updateGraphData(struct SnifferGraph &g, uint8_t value) {
  // Shift data left
  for (int i = 0; i < MAX_POINTS - 1; i++) {
    g.graphData[i] = g.graphData[i + 1];
  }
  // Add new value to the rightmost point
  g.graphData[MAX_POINTS - 1] = value;
}

void drawGraph(struct SnifferGraph &g, uint16_t pktsPerSec) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x8_tr);

  char line1[16];
  char line2[16];
  sprintf(line1, "Ch: %d", g.currentChannel);
  sprintf(line2, "Pkts/s: %d", pktsPerSec); // pktsPerSec is already scaled

  u8g2.drawStr(0, 8, line1);
  u8g2.drawStr(60, 8, line2);

  // Draw horizontal line (Base of the graph)
  u8g2.drawHLine(0, GRAPH_TOP + GRAPH_HEIGHT + 1, GRAPH_WIDTH);

  // Draw graph lines (x=0 is the first point, x=127 is the last)
  for (int x = 1; x < GRAPH_WIDTH; x++) {
    // Calculate y-coordinates (inverted: 0 is top, GRAPH_HEIGHT is bottom)
    int y1 = GRAPH_TOP + GRAPH_HEIGHT - g.graphData[x - 1];
    int y2 = GRAPH_TOP + GRAPH_HEIGHT - g.graphData[x];
    u8g2.drawLine(x - 1, y1, x, y2);
  }

  // Draw vertical marker if traffic is high
  if (pktsPerSec >= (SPIKE_THRESHOLD * 5)) { // Check against Pkts/s value
    u8g2.drawVLine(GRAPH_WIDTH - 1, GRAPH_TOP, GRAPH_HEIGHT + 2); // Draw at the current edge
  }

  u8g2.sendBuffer();
}


// --- 3. ARDUINO LOOP INTEGRATION ---

void setupSnifferGraph() {
  initDisplay();
  initSniffer(sniffer);
}

void updateSnifferGraph() {
  unsigned long now = millis();
  const unsigned long channelSwitchInterval = 1000; // 1 second
  const unsigned long updateInterval = 200; // 200 milliseconds

  // === 1. Channel Switching ===
  if (now - sniffer.lastChannelSwitch >= channelSwitchInterval) {
    sniffer.lastChannelSwitch = now;
    switchChannel(sniffer);
  }

  // === 2. Data Update and Visualization ===
  if (now - sniffer.lastUpdate >= updateInterval) {
    sniffer.lastUpdate = now;

    // CRITICAL SECTION: Safely read and reset the packet counter
    uint16_t rawPktCount;
    portENTER_CRITICAL(&snifferMux);
    rawPktCount = sniffer.packetCounter;
    sniffer.packetCounter = 0;
    portEXIT_CRITICAL(&snifferMux);
    
    // Calculate Pkts/s for display (rawPktCount / 0.2 seconds = rawPktCount * 5)
    uint16_t pktsPerSec = rawPktCount * 5; 

    // Scale the raw packet count for graph height
    // Example: If GRAPH_HEIGHT is 44, and we want 100 pkts/interval (500 Pkts/s) to be max height.
    // Scale factor: 44 / 100 = 0.44. The original code used a factor of 2.
    uint8_t scaledValue = rawPktCount * 2; // Retaining original scaling factor of 2

    // Constrain the value to the available graph height
    uint8_t value = min(scaledValue, (uint8_t)GRAPH_HEIGHT);

    updateGraphData(sniffer, value);
    drawGraph(sniffer, pktsPerSec); // Pass the final Pkts/s value for display
  }
}
