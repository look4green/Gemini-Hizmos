#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: Assuming external definitions for u8g2, ESP, heap_caps_*, and temperatureRead()
// are available and properly linked for the ESP32 platform.

// --- Global helper function placeholder (must be defined in your sketch) ---
extern "C" {
  // Standard ESP-IDF function to get chip info
  extern void esp_chip_info(esp_chip_info_t* out_info); 
  // Standard ESP-IDF function to get heap info
  extern size_t heap_caps_get_free_size(uint32_t caps); 
  extern size_t heap_caps_get_total_size(uint32_t caps);
}
// Assuming temperatureRead() is defined externally or linked from an ESP library.

void datausage(){
  // Get RAM info
  size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
  size_t totalHeap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
  
  // Handle division by zero for safety, though totalHeap should be > 0
  int ramUsage = (totalHeap > 0) ? (100 - ((freeHeap * 100) / totalHeap)) : 0; 

  // Get Flash info
  // NOTE: ESP.getFlashChipSize() returns actual chip size, 
  // but ESP.getFreeSketchSpace() is usually better for available update space.
  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t flashUsed = ESP.getSketchSize();
  
  // Handle division by zero
  int flashUsage = (flashSize > 0) ? (int)((flashUsed * 100) / flashSize) : 0; 

  // Temperature (approx)
  // Assuming temperatureRead() returns uint8_t temperature in Celsius
  uint8_t temperature = temperatureRead(); 

  // Chip info
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  // --- Drawing UI ---
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);

  u8g2.drawStr(10, 10, "ESP32-S3 Status");

  char buf[32];

  // Box 1 - RAM
  u8g2.drawFrame(0, 12, 128, 12);
  // Use integer division or floating point for more accurate display if needed, 
  // but stick to original format.
  sprintf(buf, "RAM: %d%% used (%dKB free)", ramUsage, (int)(freeHeap / 1024)); 
  u8g2.drawStr(4, 21, buf);

  // Box 2 - Flash
  u8g2.drawFrame(0, 26, 128, 12);
  sprintf(buf, "Flash: %d%% used (%dKB sketch)", flashUsage, (int)(flashUsed / 1024));
  u8g2.drawStr(4, 35, buf);

  // Box 3 - Temp
  u8g2.drawFrame(0, 40, 64, 12);
  // Added degree symbol (u8g2 supports Unicode/Glyphs, but simple C is used here)
  sprintf(buf, "Temp: %d C", temperature); 
  u8g2.drawStr(4, 49, buf);

  // Box 4 - Chip info
  u8g2.drawFrame(66, 40, 62, 12);
  // chip_info.revision is an 8-bit number representing the chip revision version.
  sprintf(buf, "Cores: %d R:%d", chip_info.cores, chip_info.revision); 
  u8g2.drawStr(68, 49, buf);

  u8g2.sendBuffer();

  // **FIX: Removed the blocking delay(500);**
}

// Example: External loop management
void mainLoop() {
    static unsigned long lastUpdateTime = 0;
    const unsigned long updateInterval = 500; // 500 milliseconds (0.5 second)

    // ... other non-blocking tasks (button checks, sensor reads, etc.) ...

    if (currentScreen == STATUS_SCREEN) {
        if (millis() - lastUpdateTime >= updateInterval) {
            datausage();
            lastUpdateTime = millis();
        }
    }
    
    // ... rest of the loop ...
}
