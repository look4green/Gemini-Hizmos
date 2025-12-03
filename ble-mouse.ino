#include <Arduino.h>
#include <U8g2lib.h>
// NOTE: Assuming external definitions for BTN_*, u8g2, mouse_ble, MOUSE_*, 
// and the debounce utility: extern bool checkButtonPress(...)

// --- Global Variables (Originals should be modified to be external or global) ---
// Assuming these are the original globals the user intended to update:
// String mouse_currentStatus = "Waiting...";
// String mouse_lastPressed = "None";

// Debounce state placeholders for single-press actions (SELECT/BACK)
static bool lastBtnSelect = HIGH;
static bool lastBtnBack = HIGH;
static unsigned long selectPressTime = 0;
static unsigned long backPressTime = 0;

// Rate Limiter for continuous movement
static unsigned long lastMoveTime = 0;
const unsigned long MOVE_RATE_MS = 20; // Move update every 20ms for smooth motion

// External XBMP definitions (must be included or defined outside the function)
// extern const unsigned char image_ArrowUpEmpty_bits[];
// extern const unsigned char image_Ble_connected_bits[];
// ... etc.

void blemouse() {
  
  // --- FIX 1: Remove local variable shadowing ---
  // The original code must be modified to ensure mouse_currentStatus and 
  // mouse_lastPressed are treated as global/external variables here.

  // Update connection status
  bool isConnected = mouse_ble.isConnected();
  
  // Check and update the global status variable
  mouse_currentStatus = isConnected ? "Connected" : "Disconnected";

  if (isConnected) {
    // --- Handle Movement (Rate-Limited, Continuous) ---
    if (millis() - lastMoveTime >= MOVE_RATE_MS) {
      if (digitalRead(BTN_UP) == LOW) {
        mouse_ble.move(0, -40);
        mouse_lastPressed = "UP";
      } else if (digitalRead(BTN_DOWN) == LOW) {
        mouse_ble.move(0, 50);
        mouse_lastPressed = "DOWN";
      } else if (digitalRead(BTN_LEFT) == LOW) {
        mouse_ble.move(-50, 0);
        mouse_lastPressed = "LEFT";
      } else if (digitalRead(BTN_RIGHT) == LOW) {
        mouse_ble.move(50, 0);
        mouse_lastPressed = "RIGHT";
      } else {
        // If no movement button is pressed, clear the last pressed status
        // But only if SELECT/BACK wasn't pressed in the same loop iteration
        // We handle click clearing separately below.
        if (mouse_lastPressed != "SELECT" && mouse_lastPressed != "BACK") {
          mouse_lastPressed = "None";
        }
      }
      lastMoveTime = millis();
    }
    
    // --- Handle Clicks (Non-blocking, Edge-Detected) ---
    // Use checkButtonPress for reliable single clicks
    if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
      mouse_ble.click(MOUSE_LEFT);
      mouse_lastPressed = "SELECT";
    } else if (checkButtonPress(BTN_BACK, lastBtnBack, backPressTime)) {
      mouse_ble.click(MOUSE_RIGHT);
      mouse_lastPressed = "BACK";
    } else if (mouse_lastPressed == "SELECT" || mouse_lastPressed == "BACK") {
      // Clear click status immediately after action if no buttons are pressed
      if (digitalRead(BTN_SELECT) != LOW && digitalRead(BTN_BACK) != LOW) {
        mouse_lastPressed = "None";
      }
    }
  }

  // --- Display with icons ---
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_6x10_tr);
  
  // Display last pressed action (e.g., UP, LEFT, SELECT)
  u8g2.setCursor(0, 10);
  u8g2.print("Last: ");
  u8g2.print(mouse_lastPressed);
  
  // BLE status icon (15x15)
  if (isConnected) {
    u8g2.drawXBMP(108, 3, 15, 15, image_Ble_connected_bits);
  } else {
    u8g2.drawXBMP(108, 3, 15, 15, image_Ble_disconnected_bits);
  }

  // Direction icons layout
  u8g2.drawXBMP(61, 9, 14, 15, image_ArrowUpEmpty_bits);     // UP
  u8g2.drawXBMP(61, 41, 14, 15, image_ArrowDownEmpty_bits);   // DOWN
  u8g2.drawXBMP(44, 25, 15, 14, image_ArrowleftEmpty_bits);   // LEFT
  u8g2.drawXBMP(76, 25, 15, 14, image_ArrowrightEmpty_bits);  // RIGHT
  u8g2.drawXBMP(60, 25, 15, 16, image_mouse_click_bits);      // CENTER (select)

  // Highlight the pressed button
  if (mouse_lastPressed == "UP") u8g2.drawFrame(60, 8, 16, 17);
  else if (mouse_lastPressed == "DOWN") u8g2.drawFrame(60, 40, 16, 17);
  else if (mouse_lastPressed == "LEFT") u8g2.drawFrame(43, 24, 17, 15);
  else if (mouse_lastPressed == "RIGHT") u8g2.drawFrame(75, 24, 17, 15);
  else if (mouse_lastPressed == "SELECT") u8g2.drawFrame(59, 24, 17, 17);
  else if (mouse_lastPressed == "BACK") {
    // Highlight the select button and display "Right Click" text
    u8g2.drawFrame(59, 24, 17, 17);
    u8g2.setCursor(0, 64);
    u8g2.print("Right Click");
  }

  u8g2.sendBuffer();
  
  // --- FIX 2: Removed the blocking delay(50); ---
}

