#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h> 
// Note: ESP32/ESP8266 WiFi libraries are assumed for WiFi.SSID(), WiFi.RSSI(), etc.

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: Replace U8G2_... with your specific display model and configuration.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// Define button pins (Placeholders)
#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_SELECT 7
#define BTN_BACK 8 

// State Variables (Must be accessible globally or defined in the main sketch)
int wifi_selectedIndex = 0;
// Note: wifi_networkCount is set by WiFi.scanNetworks() and assumed to be available.
bool wifi_showInfo = false;
extern int wifi_networkCount; // Assuming this is defined and populated elsewhere

// --- 2. UTILITY AND FORWARD DECLARATIONS ---

void runLoop(void (*handler)());
void handlewifimenu(); // Function to return to

// Dummy function for setColor (Assuming it sets the drawing color, typically handled by u8g2.setDrawColor)
void setColor(int r, int g, int b) { 
    // In u8g2 monochrome, only 0 (Black) and 1 (White) are valid for setDrawColor.
    u8g2.setDrawColor(1);
}

/**
 * @brief Waits until the specified button is released.
 * @param pin The button pin to check.
 */
void waitForRelease(uint8_t pin) {
  // Assuming buttons are pulled up (HIGH) and LOW when pressed
  while (digitalRead(pin) == LOW) {
    delay(5); // Debounce delay while held
  }
}

/**
 * @brief Maps the Wi-Fi encryption type enum to a readable string.
 */
String wifi_encryptionType(wifi_auth_mode_t encryption) {
  switch (encryption) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_MAX: return "MAX"; // Add a default case for completeness
    default: return "Unknown";
  }
}


// --- 3. THE CORRECTED scanningwifi() FUNCTION ---

void scanningwifi() {
    // --- A. Input Handling (using waitForRelease) ---
    // Handle menu navigation and selection only if networks were found
    if (wifi_networkCount > 0) { 
        if (!wifi_showInfo) {
            // Navigation on the list view
            if (digitalRead(BTN_DOWN) == LOW) {
                waitForRelease(BTN_DOWN);
                wifi_selectedIndex++;
                if (wifi_selectedIndex >= wifi_networkCount) wifi_selectedIndex = 0;
            }

            if (digitalRead(BTN_UP) == LOW) {
                waitForRelease(BTN_UP);
                wifi_selectedIndex--;
                if (wifi_selectedIndex < 0) wifi_selectedIndex = wifi_networkCount - 1;
            }

            if (digitalRead(BTN_SELECT) == LOW) {
                waitForRelease(BTN_SELECT);
                wifi_showInfo = true;
            }
        }
    }
    
    // BACK button logic is used for two purposes:
    // 1. Exit Detail View to List View (if wifi_showInfo is true)
    // 2. Exit List View to Main Wi-Fi Menu (if wifi_showInfo is false)
    if (digitalRead(BTN_BACK) == LOW) {
        waitForRelease(BTN_BACK);
        if (wifi_showInfo) {
            wifi_showInfo = false;
        } else {
            // Exit the entire scanning loop and return to the Wi-Fi Menu
            runLoop(handlewifimenu);
            return; // Exit function immediately after changing the handler
        }
    }

    // --- B. Display Rendering ---
    u8g2.clearBuffer();
    setColor(0,0,0); // Sets draw color to Black, as per the original intent (though ignored by u8g2)
    u8g2.setDrawColor(1); // Ensure default drawing is White

    if (!wifi_showInfo) {
        // ----- Main WiFi List (List View) -----
        u8g2.setFont(u8g2_font_6x13B_tr); 

        // Calculate the visible scroll window
        const int visibleItems = 4; // Based on the loop structure, 4 items are potentially visible
        int start = constrain(wifi_selectedIndex - 1, 0, wifi_networkCount - visibleItems); 
        if (wifi_selectedIndex < 2) start = 0; 
        
        int end = min(start + visibleItems, wifi_networkCount);
        
        // Handle case where few networks exist
        if (wifi_networkCount <= visibleItems) {
            start = 0;
            end = wifi_networkCount;
        }

        if (wifi_networkCount == 0) {
            u8g2.setFont(u8g2_font_7x14_tf);
            u8g2.drawStr(5, 30, "No Networks Found.");
            u8g2.setFont(u8g2_font_5x8_tr);
            u8g2.drawStr(15, 45, "Press BACK to return.");
        } else {
            for (int i = start; i < end; i++) {
                int y = (i - start) * 16;
                // Add scrolling indicator if list is longer than visible area
                if (wifi_networkCount > visibleItems) {
                    if (start > 0) u8g2.drawStr(120, 2, "^");
                    if (end < wifi_networkCount) u8g2.drawStr(120, 62, "v");
                }
                
                if (i == wifi_selectedIndex) {
                    u8g2.setDrawColor(1);
                    u8g2.drawRBox(0, y, 128, 16, 4); // Highlight
                    u8g2.setDrawColor(0); // Black text on white box
                    
                    // Display SSID, truncate if too long
                    String ssid = WiFi.SSID(i);
                    if (ssid.length() > 20) ssid = ssid.substring(0, 19) + "...";
                    u8g2.drawStr(6, y + 12, ssid.c_str());
                    
                    u8g2.setDrawColor(1); // Reset draw color
                } else {
                    String ssid = WiFi.SSID(i);
                    if (ssid.length() > 20) ssid = ssid.substring(0, 19) + "...";
                    u8g2.drawStr(6, y + 12, ssid.c_str());
                }
            }
        }
    } else {
        // ----- Detailed WiFi Info (Detail View) -----
        if (wifi_networkCount > 0) {
            u8g2.setFont(u8g2_font_5x8_tr); 
            u8g2.setDrawColor(1);

            // Box 1: SSID
            u8g2.drawFrame(0, 0, 128, 13);
            u8g2.drawStr(4, 9, ("SSID: " + WiFi.SSID(wifi_selectedIndex)).c_str());

            // Box 2: RSSI
            u8g2.drawFrame(0, 14, 128, 13);
            u8g2.drawStr(4, 23, ("RSSI: " + String(WiFi.RSSI(wifi_selectedIndex)) + " dBm").c_str());

            // Box 3: MAC (BSSID)
            u8g2.drawFrame(0, 28, 128, 13);
            u8g2.drawStr(4, 37, ("MAC: " + WiFi.BSSIDstr(wifi_selectedIndex)).c_str());

            // Box 4: Encryption
            u8g2.drawFrame(0, 42, 128, 13);
            u8g2.drawStr(4, 51, ("Enc: " + wifi_encryptionType(WiFi.encryptionType(wifi_selectedIndex))).c_str());

            // Box 5: Back hint
            u8g2.drawFrame(0, 56, 128, 8);
            u8g2.setFont(u8g2_font_4x6_tr);
            u8g2.drawStr(38, 62, "[اضغط BACK للرجوع]");
        }
    }

    u8g2.sendBuffer();
}
