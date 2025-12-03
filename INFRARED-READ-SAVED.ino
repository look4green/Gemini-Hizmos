// ==== Library Includes (Assuming these are installed) ====
#include <Arduino.h>
#include <IRremote.h>
#include <U8g2lib.h>
#include <SD.h>
#include <vector> // Required for std::vector in fileOptions

// ==== Hardware and Library Objects (Placeholder Definitions) ====
// NOTE: REPLACE THESE PIN NUMBERS AND CONSTRUCTOR WITH YOUR ACTUAL SETUP!
#define BTN_UP    14
#define BTN_DOWN  12
#define BTN_LEFT  27
#define BTN_RIGHT 26
#define BTN_SELECT 25
#define BTN_BACK  33

// IRremote Configuration
// Assuming the IR receiver/sender pins are defined in your main setup code.
// IR_RECEIVE_PIN and IR_SEND_PIN must be set for the IrReceiver and irsend to work.
// Define this value (typically 50 or 60 for IRremote library)
#define MICROS_PER_TICK 50 

// U8G2 Object (Example for 128x64 OLED via I2C)
// Replace with the constructor matching your display/connection
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// SD Card Configuration
// const int chipSelect = 5; // Example CS pin for SD card

// ==== Globals (Corrected) ====
String irmenuItems[] = {"recv ir", "ir list"};
int irMenuIndex = 0;
bool inIrSubmenu = false;
#define MAX_IR_RAW_LEN 1024
char irLastIRRaw[MAX_IR_RAW_LEN];
char irBuffer[256];
String irRawData = "";
char irInfo[6][32];

// Corrected: These should be global for `listIRFiles` and `fileOptions` to share state
String irFileList[20];
int irFileCount = 0;
// Note: If you have a global int irFileIndex, it's not used in the snippet and is redundant.

// ==== Missing Function Stubs ====

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

void runLoop(void (*drawFunction)()) {
  drawFunction();
  // In a real application, this would probably poll buttons/do background tasks
  // For this fix, we just delay briefly before returning
  delay(500); 
}

// Dummy Replay (required for replayRaw, but real definition is present below)
void replayRaw(String data); 

// Dummy Input Name (required for saveRawToFile, but real definition is present below)
String inputName();

// Dummy List IR Files (required for fileOptions, but real definition is present below)
void listIRFiles(); 

// ==== Show Message (Original) ====
void showMessage(String msg) {
  u8g2.clearBuffer();
  u8g2.setCursor(0, 20);
  u8g2.print(msg);
  u8g2.sendBuffer();
}

// ==== recvIR (Original with Fixes/Context) ====
// (The full body of recvIR is assumed to be here, but truncated for brevity)
// NOTE: Inside recvIR(), the save block calls `runLoop(drawnosdcard);`. 
// This is now fixed by defining `runLoop()` and `drawnosdcard()` above.
void recvIR() {
  // ... (Original display code for "point remote toward device" screen) ...

  // Check if back button is pressed before decode loop starts
  if (digitalRead(BTN_BACK) == LOW) {
      delay(200);
      while(digitalRead(BTN_BACK) == LOW);
      inIrSubmenu = false;
      return;
  }
  
  if (IrReceiver.decode()) {
    // ... (Original decoding and display code) ...
    
    // Save/Replay loop
    while (true) {
      if (digitalRead(BTN_BACK) == LOW) return;

      if (digitalRead(BTN_SELECT) == LOW) {
        delay(200);
        while (digitalRead(BTN_SELECT) == LOW);
        
        String name = inputName();
        if (name != "") {
          File f = SD.open("/" + name + ".ir", FILE_WRITE); // Assuming SD is initialized
          if (f) {
            f.println(irRawData);
            f.close();
            // ... (DolphinSaved success screen) ...
          } else {
            runLoop(drawnosdcard); // Fixed: Calls defined function
          }
        } else {
          showMessage("Invalid Name");
        }
        delay(1000);
        return;
      }

      if (digitalRead(BTN_UP) == LOW) {
        replayRaw(irRawData);
        // ... (Original replayed screen code) ...
        delay(2000);
        return;
      }
    }
  }
}


// ==== inputName (Original) ====
String inputName() {
  // ... (Original keyboard implementation) ...
}

// ==== saveRawToFile (Original) ====
void saveRawToFile() {
  // ... (Original logic) ...
}

// ==== Replay IR (Original) ====
void replayRaw(String data) {
  const int maxLen = 200;
  uint16_t raw[maxLen];
  int index = 0;

  char buf[data.length() + 1];
  data.toCharArray(buf, sizeof(buf));
  char *token = strtok(buf, ",");
  while (token != NULL && index < maxLen) {
    // Note: MICROS_PER_TICK is already applied when generating irRawData in recvIR
    // But the IRremote library expects the raw array to contain the cycle counts (not micros)
    // The original code passed in a string of microseconds, which is already a known issue for IRremote
    // This function assumes the string "data" already contains cycle counts or that the IRsend library handles microseconds. 
    // Since the original code used it, we leave it as is, but it might be a subtle bug.
    raw[index++] = atoi(token); 
    token = strtok(NULL, ",");
  }

  irsend.sendRaw(raw, index, 38);
}

// ==== fileOptions (Original) ====
void fileOptions(String filename) {
  // ... (Original menu and logic) ...
}

// ==== listIRFiles (Original with Scope Fix) ====
void listIRFiles() {
  // FIX: Removed local variable declarations to use the global arrays
  // String irFileList[20]; <-- REMOVED
  // int irFileCount = 0;   <-- REMOVED
  irFileCount = 0; // Reset global counter
  int selectedIndex = 0, viewOffset = 0;

  loading(); // Fixed: Calls defined function
  delay(500);

  File dir = SD.open("/");
  File entry;
  
  // FIX: Check if SD card opened the directory successfully
  if (!dir) {
      runLoop(drawnosdcard);
      inIrSubmenu = false;
      return;
  }
  
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      String name = String(entry.name());
      if (name.endsWith(".ir") && irFileCount < 20) {
        irFileList[irFileCount++] = name; // Writes to global array
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
    // ... (Original display and navigation logic) ...
    
    if (digitalRead(BTN_SELECT) == LOW) {
      delay(200); while(digitalRead(BTN_SELECT)==LOW);
      fileOptions(irFileList[selectedIndex]); // Uses globally populated list
      break;
    }

    if (digitalRead(BTN_BACK) == LOW) {
      delay(200); inIrSubmenu = false;  break;
    }

    // ... (Navigation logic) ...
  }
}
