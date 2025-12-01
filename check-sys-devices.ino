#include <Arduino.h>
#include <U8g2lib.h>
#include <SD.h>
#include <SPI.h> // Assuming radio and SD use SPI

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: Replace these with your actual display model and hardware objects/pins.

// Display Object
// extern U8G2 u8g2; 
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// Hardware Definitions (Placeholders)
#define BTN_BACK 8 // Button to exit this screen
#define SD_CS 10 // SD Card Chip Select Pin

// Assuming your radios are based on a common library (e.g., RFM9X, NRF24)
// We use a dummy class here for compilation
class DummyRadio {
public:
    bool begin(SPIClass* spi) { return true; } // Simulated successful init
    // Add real radio methods here...
};

DummyRadio radio1;
DummyRadio radio2;

// SPI Bus instances
SPIClass RADIO_SPI(VSPI); // Assuming ESP32 with VSPI for radio
SPIClass SD_SPI(HSPI); // Assuming ESP32 with HSPI for SD

// Placeholder function for LED color setting
void setColor(int r, int g, int b) {
    // Implement actual RGB LED control here
}

// --- 2. FORWARD DECLARATIONS and UTILITY FUNCTIONS ---

void runLoop(void (*handler)());
void waitForRelease(uint8_t pin);
void handlesettingsmenu(); // Function to return to

/**
 * @brief Waits until the specified button is released.
 */
void waitForRelease(uint8_t pin) {
  // Assuming buttons are pulled up (HIGH) and LOW when pressed
  while (digitalRead(pin) == LOW) {
    delay(5); 
  }
}

// --- 3. THE CORRECTED checksysdevices() FUNCTION ---

void checksysdevices() {
    // Icons provided in the original code
    static const unsigned char image_micro_sd_bits[] U8X8_PROGMEM = {
        0xfc,0x03,0x06,0x04,0x02,0x04,0x02,0x04,0xe2,0x0c,0x12,0x09,
        0xf2,0x11,0x02,0x10,0x92,0x08,0x52,0x09,0x22,0x11,0x02,0x10,
        0xe2,0x11,0x1a,0x16,0xec,0x0d,0x00,0x00
    };
    static const unsigned char image_satellite_dish_bits[] U8X8_PROGMEM = {
        0x00,0x00,0x00,0x1e,0x0c,0x20,0x34,0x4c,0xc6,0x50,0x15,0x57,
        0x49,0x45,0x09,0x07,0x91,0x08,0x51,0x12,0x22,0x10,0xc2,0x24,
        0x04,0x23,0x0c,0x3c,0x30,0x08,0xc0,0x07
    };

    // States are calculated only once when the function is entered
    static bool initialization_done = false;
    static String radio1_state = "error";
    static String radio2_state = "error";
    static String sd_state     = "error";

    // --- Perform Initialization/Checks only once ---
    if (!initialization_done) {
        // Init radio1
        if (radio1.begin(&RADIO_SPI)) {
            radio1_state = "work";
        }

        // Init radio2
        if (radio2.begin(&RADIO_SPI)) {
            radio2_state = "work";
        }

        // Init SD Card
        // NOTE: The `setColor(0,255,0)` line in the original code is now placed 
        // inside the check for success.
        if (SD.begin(SD_CS, SD_SPI)) {
            sd_state = "work";
            setColor(0, 255, 0); // Green light for success
        }

        initialization_done = true;
    }

    // --- Input Handling (Exit) ---
    // If BTN_BACK is pressed, exit to the settings menu
    if (digitalRead(BTN_BACK) == LOW) {
        waitForRelease(BTN_BACK);
        runLoop(handlesettingsmenu);
        return; // Exit the function loop
    }

    // ----- عرض النتائج (Display Results) -----
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    
    // Title
    u8g2.setFont(u8g2_font_7x14_tf);
    u8g2.drawStr(10, 12, "Device Status");
    u8g2.drawHLine(0, 15, 128); // Separator line

    // أيقونات (Icons) - Y position adjusted for title
    u8g2.drawXBMP(10, 30, 14, 16, image_micro_sd_bits);       // SD Icon
    u8g2.drawXBMP(47, 30, 15, 16, image_satellite_dish_bits); // Radio1 Icon
    u8g2.drawXBMP(91, 30, 15, 16, image_satellite_dish_bits); // Radio2 Icon

    // تسميات (Labels)
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(13, 27, "SD");
    u8g2.drawStr(43, 27, "RADIO1");
    u8g2.drawStr(87, 27, "RADIO2");

    // الحالات (States) - Adjusted Y position
    u8g2.setFont(u8g2_font_6x10_tr); 

    // Draw Status Strings (Work/Error)
    u8g2.drawStr(5,  56, sd_state.c_str());
    u8g2.drawStr(42, 56, radio1_state.c_str());
    u8g2.drawStr(86, 56, radio2_state.c_str());

    // Hint for user
    u8g2.setFont(u8g2_font_profont10_tr);
    u8g2.drawStr(0, 64, "Press BACK to exit");

    u8g2.sendBuffer();
}
