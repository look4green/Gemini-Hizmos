// =======================================================
//                    IR DOLPHIN CLONE
// =======================================================

// --- LIBRARIES ---
#include <Arduino.h>
#include <IRremote.h>
#include <U8g2lib.h>
#include <SD.h>
#include <vector>

// --- PIN DEFINITIONS (MUST BE ADJUSTED FOR YOUR BOARD) ---
#define BTN_UP      14
#define BTN_DOWN    12
#define BTN_SELECT  25
#define BTN_BACK    33
// The inputName function also uses LEFT/RIGHT, but we'll map them for simplicity
#define BTN_LEFT    27 
#define BTN_RIGHT   26 

// IR Pins
#define IR_RECEIVE_PIN 15  // Pin for IR Receiver Module
#define IR_SEND_PIN    4   // Pin for IR Transmitter LED

// SD Card Chip Select Pin
#define SD_CS_PIN      5 

// IR Timing Constant (Standard for IRremote library)
#define MICROS_PER_TICK 50 

// --- U8G2 OBJECT ---
// Adjust the constructor to match your display model (e.g., SSD1306) and interface (I2C/SPI)
// Example: 128x64 OLED via I2C (SCL=22, SDA=21 on many ESP32 boards)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// --- GLOBALS ---
String irmenuItems[] = {"recv ir", "ir list"};
int irMenuIndex = 0;
bool inIrSubmenu = false; // Controls if we are in a main function or the main menu

#define MAX_IR_RAW_LEN 1024
char irLastIRRaw[MAX_IR_RAW_LEN];
String irRawData = "";

// Global file list for sharing between listIRFiles and fileOptions
String irFileList[20];
int irFileCount = 0;

// =======================================================
//                  MISSING BITMAPS (STUBS)
//  NOTE: You must paste the U8X8_PROGMEM bitmap arrays
//  from your original code here (e.g., image_hizmos_oled_bits)
//  For now, we define them as stubs to allow compilation.
// =======================================================

static const unsigned char image_hizmos_oled_bits[] U8X8_PROGMEM = { /* ... paste data here ... */ 0x01};
static const unsigned char image_DolphinReadingSuccess_bits[] U8X8_PROGMEM = { /* ... paste data here ... */ 0x01};
static const unsigned char image_KeyBackspaceSelected_bits[] U8X8_PROGMEM = { /* ... paste data here ... */ 0x01};
static const unsigned char image_KeySaveSelected_bits[] U8X8_PROGMEM = { /* ... paste data here ... */ 0x01};
static const unsigned char image_Untitled_bits[] U8X8_PROGMEM = { /* ... paste data here ... */ 0x01};
static const unsigned char image_download_bits[] U8X8_PROGMEM = { /* ... paste data here ... */ 0x01};
static const unsigned char image_DolphinSaved_bits[] U8X8_PROGMEM = { /* ... paste data here ... */ 0x01};
static const unsigned char image_FILLED_bits[] U8X8_PROGMEM = { /* ... paste data here ... */ 0x01};


// =======================================================
//                 UTILITY AND STUB FUNCTIONS
// =======================================================

// Forward Declarations
void listIRFiles(); 
void fileOptions(String filename);
String inputName();
void replayRaw(String data);

void showMessage(String msg) {
  u8g2.clearBuffer();
  u8g2.setCursor(0, 20);
  u8g2.print(msg);
  u8g2.sendBuffer();
}

void drawnosdcard() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_haxrcorp4089_tr);
  u8g2.drawStr(10, 30, "NO SD CARD!");
  u8g2.sendBuffer();
  delay(1000);
}

void loading() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_haxrcorp4089_tr);
  u8g2.drawStr(10, 30, "LOADING...");
  u8g2.sendBuffer();
}

// Simple placeholder for runLoop, called on SD error
void runLoop(void (*drawFunction)()) {
  drawFunction();
  delay(500); 
}

// =======================================================
//                 CORE IR FUNCTIONS
// =======================================================

void replayRaw(String data) {
  const int maxLen = 200;
  uint16_t raw[maxLen];
  int index = 0;

  char buf[data.length() + 1];
  data.toCharArray(buf, sizeof(buf));
  char *token = strtok(buf, ",");
  while (token != NULL && index < maxLen) {
    // NOTE: The original code parsed microseconds; IRremote may expect ticks.
    // We maintain the original behavior for compatibility with the rest of the code.
    raw[index++] = atoi(token);
    token = strtok(NULL, ",");
  }

  // Uses the globally available irsend object
  irsend.sendRaw(raw, index, 38); 
}

// =======================================================
//                 INPUT NAME (KEYBOARD)
// =======================================================

String inputName() {
  // ... (Full implementation of inputName from your original code) ...
  const char* keys[] = {
    "<","A","B","C","D","E","F","G",
    "H","I","J","K","L","M","N","O",
    "P","Q","R","S","T","U","V","W",
    "X","Y","Z","0","1","2","3","4",
    "5","6","7","8","9","-","_","*",
    "/","@","#",".",",","ENTER"
  };

  const int totalKeys = sizeof(keys) / sizeof(keys[0]);
  const int cols = 8;
  const int rows = (totalKeys + cols - 1) / cols;
  const int visibleRows = 3;

  int offsetY = 0;
  int cursorX = 0, cursorY = 0;
  String name = "";

  const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  // Generates a random initial name
  for (int i = 0; i < 6; i++) name += charset[random(strlen(charset))];

  while (true) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tf);

    // Draw name
    u8g2.drawStr(0, 10, "NAME:");
    u8g2.drawStr(42, 10, name.c_str());

    // Draw keys
    for (int y = 0; y < visibleRows; y++) {
      for (int x = 0; x < cols; x++) {
        int keyIndex = (offsetY + y) * cols + x;
        if (keyIndex >= totalKeys) continue;

        int boxX = x * 16;
        int boxY = 14 + y * 16;

        const char* key = keys[keyIndex];
        int boxW = (strcmp(key, "ENTER") == 0) ? 30 : 14;

        if (x == cursorX && y == cursorY) {
          u8g2.drawBox(boxX, boxY, boxW, 14);
          u8g2.setDrawColor(0);
          u8g2.setCursor(boxX + 2, boxY + 11);
          u8g2.print(key);
          u8g2.setDrawColor(1);
        } else {
          u8g2.drawFrame(boxX, boxY, boxW, 14);
          u8g2.setCursor(boxX + 2, boxY + 11);
          u8g2.print(key);
        }
      }
    }

    u8g2.sendBuffer();

    // Controls
    if (digitalRead(BTN_UP) == LOW) {
      if (cursorY == 0 && offsetY > 0) {
        offsetY--;
      } else if (cursorY > 0) {
        cursorY--;
      }
      delay(150);
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      if ((cursorY + offsetY + 1) * cols + cursorX < totalKeys && cursorY < visibleRows - 1) {
        cursorY++;
      } else if (offsetY + visibleRows < rows) {
        offsetY++;
      }
      delay(150);
    }
    if (digitalRead(BTN_LEFT) == LOW) {
      if (cursorX > 0) cursorX--;
      delay(150);
    }
    if (digitalRead(BTN_RIGHT) == LOW) {
      if ((offsetY + cursorY) * cols + cursorX + 1 < totalKeys && cursorX < cols - 1) cursorX++;
      delay(150);
    }
    if (digitalRead(BTN_SELECT) == LOW) {
      int keyIndex = (cursorY + offsetY) * cols + cursorX;
      if (keyIndex < totalKeys) {
        const char* key = keys[keyIndex];
        if (strcmp(key, "<") == 0) {
          if (name.length() > 0) name.remove(name.length() - 1);
        } else if (strcmp(key, "ENTER") == 0) {
          return name;
        } else {
          const int maxNameLen = 10; // Defined locally within the original code
          if (name.length() < maxNameLen) name += key;
        }
      }
      delay(200);
    }
    if (digitalRead(BTN_BACK) == LOW) {
      return ""; // Cancel input
    }
  }
}

// =======================================================
//                     RECV IR
// =======================================================

void recvIR() {
  // ... (Full implementation of recvIR from your original code) ...
  
  // Waiting Screen
  u8g2.clearBuffer();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.drawXBMP(86, 5, 37, 53, image_hizmos_oled_bits);
  u8g2.setFont(u8g2_font_profont12_tr);
  u8g2.drawStr(2, 12, "point remote ");
  u8g2.drawStr(1, 28, "toward device");
  u8g2.drawStr(1, 42, "then press ");
  u8g2.drawStr(2, 56, "a button ");
  u8g2.sendBuffer();

  // Check for BTN_BACK press to exit
  if (digitalRead(BTN_BACK) == LOW) {
      delay(200);
      while(digitalRead(BTN_BACK) == LOW);
      inIrSubmenu = false;
      return;
  }
  
  // Attempt to decode IR signal
  if (IrReceiver.decode()) {
    // --- Decoding successful ---

    uint32_t hex = IrReceiver.decodedIRData.decodedRawData;
    uint16_t addr = IrReceiver.decodedIRData.address;
    uint16_t cmd = IrReceiver.decodedIRData.command;
    uint8_t bits = IrReceiver.decodedIRData.numberOfBits;
    uint8_t rawlen = IrReceiver.decodedIRData.rawDataPtr->rawlen;
    const char* type = IrReceiver.getProtocolString();

    // Convert numbers to text
    char addrStr[10];
    char cmdStr[10];
    sprintf(addrStr, "%04X", addr);
    sprintf(cmdStr, "%04X", cmd);

    // Success Display (DolphinReadingSuccess)
    u8g2.clearBuffer();
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.drawXBMP(0, 0, 59, 63, image_DolphinReadingSuccess_bits);
    u8g2.setFont(u8g2_font_haxrcorp4089_tr);
    u8g2.drawStr(72, 42, "c:");
    u8g2.drawStr(72, 29, "a:");
    u8g2.setFont(u8g2_font_profont15_tr);
    u8g2.drawStr(81, 15, type); // Protocol
    u8g2.setFont(u8g2_font_haxrcorp4089_tr);
    u8g2.drawStr(81, 29, addrStr); // Address
    u8g2.drawStr(81, 43, cmdStr); // Command
    u8g2.drawXBMP(49, 53, 24, 11, image_KeySaveSelected_bits);
    u8g2.drawXBMP(0, 54, 16, 9, image_KeyBackspaceSelected_bits);
    u8g2.drawXBMP(94, 53, 34, 11, image_Untitled_bits);
    u8g2.sendBuffer();

    // Store Raw Data
    irRawData = "";
    for (uint8_t i = 1; i < rawlen; i++) {
      // Data is stored as microseconds
      irRawData += String(IrReceiver.decodedIRData.rawDataPtr->rawbuf[i] * MICROS_PER_TICK);
      if (i < rawlen - 1) irRawData += ",";
    }

    IrReceiver.resume(); // Resume IR to listen for next signal

    // --- Post-Reception Loop (Save/Replay) ---
    while (true) {
      if (digitalRead(BTN_BACK) == LOW) return;

      if (digitalRead(BTN_SELECT) == LOW) {
        delay(200);
        while (digitalRead(BTN_SELECT) == LOW);
        
        String name = inputName();
        if (name != "") {
          File f = SD.open("/" + name + ".ir", FILE_WRITE);
          if (f) {
            f.println(irRawData);
            f.close();
            // Success Screen (DolphinSaved)
            u8g2.clearBuffer(); u8g2.setFontMode(1); u8g2.setBitmapMode(1);
            u8g2.drawXBMP(0, 0, 128, 64, image_FILLED_bits);
            u8g2.setDrawColor(2);
            u8g2.drawXBMP(36, 6, 92, 58, image_DolphinSaved_bits);
            u8g2.setFont(u8g2_font_haxrcorp4089_tr);
            u8g2.drawStr(9, 17, "SAVED!");
            u8g2.sendBuffer();
          } else {
            runLoop(drawnosdcard); // SD Card Save Failed
          }
        } else {
          showMessage("Invalid Name");
        }
        delay(1000);
        return;
      }

      if (digitalRead(BTN_UP) == LOW) {
        replayRaw(irRawData);
        // Replay Success Screen (download icon)
        u8g2.clearBuffer(); u8g2.setFontMode(1); u8g2.setBitmapMode(1);
        u8g2.drawXBMP(48, 5, 80, 58, image_download_bits);
        u8g2.setFont(u8g2_font_profont12_tr);
        u8g2.drawStr(5, 30, "replayed");
        u8g2.setFont(u8g2_font_profont15_tr);
        u8g2.drawStr(8, 14, "signal");
        u8g2.sendBuffer();
        delay(2000);
        return;
      }
    }
  }
}

// =======================================================
//                      FILE OPTIONS
// =======================================================

void fileOptions(String filename) {
  const String opts[] = {"Run", "Rename", "Delete", "Info"};
  int optCount = 4, optIndex = 0;

  while (true) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, filename.c_str());

    // Display menu
    for (int i = 0; i < optCount; i++) {
      int y = 22 + i * 12;
      if (i == optIndex) {
        u8g2.drawBox(0, y - 10, 128, 12);
        u8g2.setDrawColor(0);
      } else {
        u8g2.setDrawColor(1);
      }
      u8g2.setCursor(4, y);
      u8g2.print(opts[i]);
    }
    u8g2.setDrawColor(1);
    u8g2.sendBuffer();

    // Controls
    if (digitalRead(BTN_UP) == LOW) {
      optIndex = (optIndex + optCount - 1) % optCount;
      delay(200);
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      optIndex = (optIndex + 1) % optCount;
      delay(200);
    }

    // Execute Selection
    if (digitalRead(BTN_SELECT) == LOW) {
      delay(200); while (digitalRead(BTN_SELECT) == LOW);
      String selected = opts[optIndex];

      if (selected == "Run") {
        File f = SD.open("/" + filename);
        if (f) {
          String raw = f.readStringUntil('\n'); f.close();
          replayRaw(raw); // raw.c_str() is implicit in the call
          showMessage("Replayed");
        } else showMessage("File Error");
      }
      else if (selected == "Rename") {
        String newname = inputName();
        if (newname != "") {
          SD.rename("/" + filename, "/" + newname + ".ir");
          showMessage("Renamed");
          filename = newname + ".ir";
        } else showMessage("Invalid");
      }
      else if (selected == "Delete") {
        SD.remove("/" + filename);
        showMessage("Deleted");
        delay(800);
        break; // Exit to file list
      }
      else if (selected == "Info") {
        File f = SD.open("/" + filename);
        if (!f) { showMessage("Can't open"); continue; }

        std::vector<String> lines;
        while (f.available()) {
          String line = f.readStringUntil('\n');
          line.trim();
          lines.push_back(line);
        }
        f.close();

        int scroll = 0;
        const int visibleLines = 5;
        int totalLines = lines.size();

        while (true) {
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_6x10_tf);
          u8g2.drawStr(0, 10, "Content:");

          // Display lines
          for (int i = 0; i < visibleLines; i++) {
            int index = scroll + i;
            if (index < totalLines) {
              u8g2.drawStr(0, 22 + i * 10, lines[index].c_str());
            }
          }
          // Scroll bar... (original scrollbar code) ...
          if (totalLines > visibleLines) {
             int scrollbarHeight = 40;
             int barY = 22;
             int barX = 124;
             int barH = (visibleLines * scrollbarHeight) / totalLines;
             int barPos = (scroll * scrollbarHeight) / totalLines;
             u8g2.drawFrame(barX, barY, 4, scrollbarHeight);
             u8g2.drawBox(barX, barY + barPos, 4, barH);
          }
          u8g2.sendBuffer();

          if (digitalRead(BTN_DOWN) == LOW && scroll + visibleLines < totalLines) { scroll++; delay(150); }
          if (digitalRead(BTN_UP) == LOW && scroll > 0) { scroll--; delay(150); }
          if (digitalRead(BTN_BACK) == LOW) { delay(200); while (digitalRead(BTN_BACK) == LOW); break; }
        }
      }
    }

    if (digitalRead(BTN_BACK) == LOW) {
      delay(200); while (digitalRead(BTN_BACK) == LOW);
      break;
    }
  }

  listIRFiles();
}

// =======================================================
//                     LIST IR FILES
// =======================================================

void listIRFiles() {
  // Use global variables
  irFileCount = 0; 
  int selectedIndex = 0, viewOffset = 0;

  loading();
  delay(500);

  File dir = SD.open("/");
  
  if (!dir) { // Check if SD is initialized/accessible
      runLoop(drawnosdcard);
      inIrSubmenu = false;
      return;
  }

  File entry;
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      String name = String(entry.name());
      if (name.endsWith(".ir") && irFileCount < 20) {
        irFileList[irFileCount++] = name;
      }
    }
    entry.close();
  }
  dir.close();

  if (irFileCount == 0) {
    showMessage("No files");
    delay(1000);
    inIrSubmenu = false;
    return;
  }

  const int itemHeight = 14;
  const int visibleItems = 4;

  while (true) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);

    for (int i = 0; i < visibleItems; i++) {
      int fi = viewOffset + i;
      if (fi >= irFileCount) break;

      int y = i * itemHeight + 14;

      if (fi == selectedIndex) {
        u8g2.setDrawColor(1);
        u8g2.drawRBox(0, y - 11, 128, itemHeight, 3);
        u8g2.setDrawColor(0);
      } else {
        u8g2.setDrawColor(1);
      }

      u8g2.setCursor(6, y - 2);
      u8g2.print(irFileList[fi]);
    }

    u8g2.sendBuffer();

    // Controls
    if (digitalRead(BTN_SELECT) == LOW) {
      delay(200); while(digitalRead(BTN_SELECT)==LOW);
      fileOptions(irFileList[selectedIndex]);
      break;
    }

    if (digitalRead(BTN_BACK) == LOW) {
      delay(200); inIrSubmenu = false;  break;
    }

    if (digitalRead(BTN_UP) == LOW) {
      if (selectedIndex > 0) selectedIndex--;
      if (selectedIndex < viewOffset) viewOffset--;
      delay(150);
    }

    if (digitalRead(BTN_DOWN) == LOW) {
      if (selectedIndex < irFileCount - 1) selectedIndex++;
      if (selectedIndex > viewOffset + visibleItems - 1) viewOffset++;
      delay(150);
    }
  }
}

// =======================================================
//                      MAIN MENU DRAW
// =======================================================

void drawMainMenu() {
  const int itemHeight = 14;
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "IR Remote Utility");

  for (int i = 0; i < 2; i++) {
    int y = i * itemHeight + 28;
    
    if (i == irMenuIndex) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, y - 11, 128, itemHeight);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1);
    }
    
    u8g2.setCursor(6, y - 2);
    u8g2.print(irmenuItems[i]);
  }
  u8g2.setDrawColor(1);
  u8g2.sendBuffer();

  // Navigation Logic for Main Menu
  if (digitalRead(BTN_UP) == LOW) {
    irMenuIndex = (irMenuIndex + 2 - 1) % 2;
    delay(200);
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    irMenuIndex = (irMenuIndex + 1) % 2;
    delay(200);
  }
  if (digitalRead(BTN_SELECT) == LOW) {
    delay(200);
    while(digitalRead(BTN_SELECT) == LOW);
    inIrSubmenu = true;
  }
}


// =======================================================
//                  ARDUINO FRAMEWORK
// =======================================================

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0)); // Initialize random seed for inputName

  // --- Initialize U8G2 Display ---
  u8g2.begin();
  u8g2.setDrawColor(1);
  u8g2.setFontMode(1);

  // --- Initialize Buttons ---
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  // --- Initialize IR & SD ---
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // Start the IR Receiver
  irsend.begin(IR_SEND_PIN); // Start the IR Transmitter

  // --- Initialize SD Card ---
  if (!SD.begin(SD_CS_PIN)) {
    showMessage("SD Failed!");
    delay(2000);
  } else {
    showMessage("SD Ready!");
    delay(1000);
  }
}

void loop() {
  // Check if we are inside a specific function (recv ir or ir list)
  if (inIrSubmenu) {
    if (irMenuIndex == 0) {
      recvIR();
    } else if (irMenuIndex == 1) {
      listIRFiles();
    }
  } else {
    // We are in the main menu
    drawMainMenu();
  }
}
