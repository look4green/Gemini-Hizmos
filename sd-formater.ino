#include <Arduino.h>
#include <U8g2lib.h>
#include <SD.h> 
#include <FS.h> 

// --- 1. GLOBAL CONTEXT & STATE VARIABLES ---
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

#define BTN_SELECT 7
#define BTN_BACK 8 

// ==== Format State ====
enum FormaterState {
  FORMAT_ASK,
  FORMAT_CONFIRM,
  FORMAT_PROGRESS,
  FORMAT_DELETING, // New state for non-blocking file deletion
  FORMAT_DONE
};

FormaterState formaterState = FORMAT_ASK;

// Debouncing state
static bool formaterLastSelectState = HIGH;
static bool formaterLastBackState = HIGH;
static unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; 

// Progress variables for non-blocking operations
unsigned long progressStartTime = 0;
const unsigned long totalFormatDuration = 2000; // 2 seconds for fake progress bar
const unsigned long doneDisplayDuration = 2000; // 2 seconds for "DONE" screen
const int progressBoxWidth = 108;

// Deletion iterator
File rootDir;
bool isDeleting = false; 

// Forward declarations
void sdformater_drawBoxMessage(const char* line1, const char* line2 = nullptr, const char* footer = nullptr);
void sdformater_drawProgress(int progress);
void sdformater_deleteNext(File dir);


// --- 2. DRAWING FUNCTIONS ---

// ==== رسم رسالة داخل بوكس دائري (Helper updated for generic use) ====
void sdformater_drawBoxMessage(const char* line1, const char* line2, const char* footer) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1); // Set to white for frame

  u8g2.drawRBox(6, 14, 116, 36, 6);  // Box
  u8g2.setDrawColor(0); // Set to black for text

  if (line1) {
    int w1 = u8g2.getStrWidth(line1);
    u8g2.setCursor((128 - w1) / 2, 28);
    u8g2.print(line1);
  }

  if (line2) {
    int w2 = u8g2.getStrWidth(line2);
    u8g2.setCursor((128 - w2) / 2, 40);
    u8g2.print(line2);
  }

  u8g2.setDrawColor(1); // Reset to white
  
  if (footer) {
    int wf = u8g2.getStrWidth(footer);
    u8g2.setCursor((128 - wf) / 2, 62);
    u8g2.print(footer);
  }

  u8g2.sendBuffer();
}

// ==== شريط التحميل ====
void sdformater_drawProgress(int progress) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(34, 15, "Formatting...");
  u8g2.drawFrame(10, 30, progressBoxWidth, 12);
  u8g2.drawBox(10, 30, progress, 12);
  u8g2.sendBuffer();
}


// --- 3. CORE LOGIC (Non-blocking File Deletion) ---

// ==== حذف كل الملفات (Recursive, non-blocking approach is complex, but the original blocking one is refactored below) ====
// We modify the original function to be called iteratively, or stick to the original structure 
// but ensure the outer loop calling this is only run once per state, not repeatedly.
// Since the original was BLOCKING, we adapt it to run *once* when state FORMAT_DELETING is entered.
void formaterDeleteAllFromSD_Blocking(File dir) {
  // NOTE: This function is still blocking but is called from a new, simple state (FORMAT_DELETING) 
  // which is then immediately followed by the non-blocking FORMAT_DONE state.
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    if (entry.isDirectory()) {
      formaterDeleteAllFromSD_Blocking(entry); // Recursive call
      SD.rmdir(entry.name());
    } else {
      SD.remove(entry.name());
    }

    entry.close();
  }
  dir.close();
}


// --- 4. STATE HANDLERS (Non-blocking) ---

// ==== تنفيذ عملية الفورمات (Renamed to be non-blocking trigger) ====
void formaterTrigger() {
    progressStartTime = millis();
    formaterState = FORMAT_PROGRESS;
}

// ==== Main Update Loop ====
void sdformater() {
    unsigned long now = millis();
    bool selectPressed = digitalRead(BTN_SELECT) == LOW;
    bool backPressed = digitalRead(BTN_BACK) == LOW;
    
    // --- 4.1 Input Handling (Non-blocking Debounce) ---
    // Check for state change
    if (selectPressed != formaterLastSelectState || backPressed != formaterLastBackState) {
        lastDebounceTime = now;
    }
    
    // Process input after debounce time
    if ((now - lastDebounceTime) > debounceDelay) {
        // SELECT button logic (Press event: HIGH -> LOW)
        if (selectPressed && formaterLastSelectState == HIGH) {
            switch (formaterState) {
                case FORMAT_ASK:
                    formaterState = FORMAT_CONFIRM;
                    sdformater_drawBoxMessage("Are you sure?", "Data will be erased!", "SELECT=Yes  BACK=No");
                    break;
        
                case FORMAT_CONFIRM:
                    formaterTrigger(); // Start progress
                    break;
                
                default:
                    // If pressed during PROGRESS/DONE, ignore or could potentially be used to exit early
                    break;
            }
        }

        // BACK button logic (Press event: HIGH -> LOW)
        if (backPressed && formaterLastBackState == HIGH) {
            if (formaterState == FORMAT_CONFIRM) {
                formaterState = FORMAT_ASK;
                sdformater_drawBoxMessage("Do you want to", "format SD card?", "Press SELECT to continue");
            }
            // Add case to exit the utility completely here if needed.
        }
        
        // Update last state for next loop iteration
        formaterLastSelectState = selectPressed;
        formaterLastBackState = backPressed;
    }


    // --- 4.2 State Machine Update (Non-blocking logic) ---
    switch (formaterState) {
        case FORMAT_ASK:
            // Ensure the ASK screen is drawn if we just entered the utility
            static bool needsInitialDraw = true;
            if (needsInitialDraw) {
                sdformater_drawBoxMessage("Do you want to", "format SD card?", "Press SELECT to continue");
                needsInitialDraw = false;
            }
            break;

        case FORMAT_PROGRESS: {
            // Non-blocking progress bar simulation
            unsigned long elapsed = now - progressStartTime;
            int progress = map(elapsed, 0, totalFormatDuration, 0, progressBoxWidth);
            
            sdformater_drawProgress(constrain(progress, 0, progressBoxWidth));

            if (elapsed >= totalFormatDuration) {
                // Progress bar finished, move to actual deletion (Blocking step)
                formaterState = FORMAT_DELETING;
            }
            break;
        }

        case FORMAT_DELETING: {
            // This is the blocking part, but it runs only ONCE.
            sdformater_drawProgress(progressBoxWidth); // Keep bar full
            File root = SD.open("/");
            if (root) {
                formaterDeleteAllFromSD_Blocking(root);
            }
            progressStartTime = now; // Reuse timer for DONE screen duration
            formaterState = FORMAT_DONE;
            break;
        }

        case FORMAT_DONE: {
            // Non-blocking display of DONE message
            sdformater_drawBoxMessage("SD Card", "Formatted!", "Returning...");
            
            if (now - progressStartTime >= doneDisplayDuration) {
                formaterState = FORMAT_ASK; // Reset state
                needsInitialDraw = true;
            }
            break;
        }
        
        default:
            break;
    }
}
