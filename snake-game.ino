#include <Arduino.h>
#include <U8g2lib.h>
#include <vector> // Required for std::vector<Point>

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: Replace U8G2_... with your specific display model and configuration.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// Define button pins (Placeholders)
#define BTN_UP 5
#define BTN_DOWN 6
#define BTN_LEFT 7
#define BTN_RIGHT 8
#define BTN_SELECT 9
#define BTN_BACK 10 

// Game Grid Definitions (from original code)
#define GRID_SIZE 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define GRID_COLS (SCREEN_WIDTH / GRID_SIZE)
#define GRID_ROWS (SCREEN_HEIGHT / GRID_SIZE)

// --- 2. GAME STATE VARIABLES ---
enum Direction { UP, DOWN, LEFT, RIGHT };
Direction dir = RIGHT;

struct Point { int x, y; };

std::vector<Point> snake;
Point food;
unsigned long lastMoveTime = 0;
int moveDelay = 200; // Speed (ms)
bool gameOver = false;
int score = 0;
bool gameStarted = false;

// Debouncing variables
unsigned long lastInputTime = 0;
const unsigned long debounceDelay = 150; // Delay to prevent double clicks


// --- 3. UTILITY FUNCTIONS ---

/**
 * @brief Checks if a button is pressed and handles debouncing.
 * Returns true only on a state change (press), preventing repeated actions.
 * @param pin The button pin to check.
 * @return true if the button is pressed and debounce time has passed.
 */
bool checkButtonPress(int pin) {
  if (digitalRead(pin) == LOW) {
    if (millis() - lastInputTime > debounceDelay) {
      lastInputTime = millis();
      return true;
    }
  }
  return false;
}

// Function to handle the actual start of the game
void startGame(); 
void placeFood();

// --- 4. SCREEN DRAWING FUNCTIONS ---

void drawStartScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(25, 20, "SNAKE GAME");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(15, 40, "Press SELECT to start");
  u8g2.sendBuffer();
}

void drawGameOverScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(20, 20, "GAME OVER");
  u8g2.setFont(u8g2_font_6x10_tr);
  char buf[20];
  sprintf(buf, "Score: %d", score);
  u8g2.drawStr(35, 35, buf);
  u8g2.drawStr(5, 55, "SELECT: Retry   BACK: Exit");
  u8g2.sendBuffer();
}

void placeFood() {
  while (true) {
    food.x = random(0, GRID_COLS);
    food.y = random(0, GRID_ROWS);
    bool onSnake = false;
    for (const auto& part : snake) {
      if (part.x == food.x && part.y == food.y) {
        onSnake = true;
        break;
      }
    }
    if (!onSnake) break;
  }
}

void snakeSetup() {
  randomSeed(millis());
  // Initialization call added here (normally in Arduino setup())
  // u8g2.begin(); 
  drawStartScreen();
}

void startGame() {
  snake.clear();
  snake.push_back({GRID_COLS / 2, GRID_ROWS / 2});
  dir = RIGHT;
  gameOver = false;
  score = 0;
  placeFood();
  lastMoveTime = millis();
  gameStarted = true;
}

// --- 5. MAIN GAME LOOP (Corrected) ---

void snakeLoop() {
    
  // --- A. START SCREEN LOGIC ---
  if (!gameStarted) {
    if (checkButtonPress(BTN_SELECT)) {
      startGame();
    }
    return;
  }

  // --- B. GAME OVER LOGIC (Non-blocking) ---
  if (gameOver) {
    drawGameOverScreen();
    if (checkButtonPress(BTN_SELECT)) {
      startGame();
    } else if (checkButtonPress(BTN_BACK)) {
      gameStarted = false;
      drawStartScreen();
    }
    return;
  }

  // --- C. IN-GAME INPUT (Movement) ---
  // Note: Using checkButtonPress prevents accidental double-moves/fast-scrolling
  // The 'else if' structure is fine for preventing 180-degree turns and limiting to 90-degree turns.
  if (checkButtonPress(BTN_UP) && dir != DOWN) dir = UP;
  else if (checkButtonPress(BTN_DOWN) && dir != UP) dir = DOWN;
  else if (checkButtonPress(BTN_LEFT) && dir != RIGHT) dir = LEFT;
  else if (checkButtonPress(BTN_RIGHT) && dir != LEFT) dir = RIGHT;

  // --- D. GAME LOGIC (Time-based movement) ---
  if (millis() - lastMoveTime < moveDelay) return;
  lastMoveTime = millis();

  // 1. تحديث موقع الرأس
  Point head = snake[0];
  switch (dir) {
    case UP: head.y--; break;
    case DOWN: head.y++; break;
    case LEFT: head.x--; break;
    case RIGHT: head.x++; break;
  }

  // 2. التحقق من الاصطدام (الجدران)
  if (head.x < 0 || head.y < 0 || head.x >= GRID_COLS || head.y >= GRID_ROWS) {
    gameOver = true;
    return;
  }
  // 3. التحقق من الاصطدام (بالجسم)
  for (size_t i = 1; i < snake.size(); i++) { // Start check from index 1 (the neck)
    if (snake[i].x == head.x && snake[i].y == head.y) {
      gameOver = true;
      return;
    }
  }

  // 4. تحديث الجسم (إضافة رأس جديد)
  snake.insert(snake.begin(), head);

  // 5. أكل التفاحة
  if (head.x == food.x && head.y == food.y) {
    score++;
    // Increase speed by reducing moveDelay (optional but common)
    if (moveDelay > 50) moveDelay -= 5; 
    placeFood();
  } else {
    snake.pop_back(); // إزالة الذيل إذا لم يأكل
  }

  // --- E. رسم كل شيء ---
  u8g2.clearBuffer();

  // النقاط
  u8g2.setFont(u8g2_font_5x8_tr);
  char scoreStr[10];
  sprintf(scoreStr, "Score: %d", score);
  u8g2.drawStr(2, 8, scoreStr);

  // التفاحة (دائرة)
  u8g2.drawDisc(food.x * GRID_SIZE + GRID_SIZE / 2,
                food.y * GRID_SIZE + GRID_SIZE / 2,
                GRID_SIZE / 2);

  // جسم الثعبان
  for (size_t i = 1; i < snake.size(); i++) {
    u8g2.drawBox(snake[i].x * GRID_SIZE, snake[i].y * GRID_SIZE, GRID_SIZE, GRID_SIZE);
  }

  // رأس الثعبان (مربع مميز)
  u8g2.drawFrame(snake[0].x * GRID_SIZE, snake[0].y * GRID_SIZE, GRID_SIZE, GRID_SIZE);

  u8g2.sendBuffer();
}

// Example setup loop if this were a complete Arduino sketch:
/*
void setup() {
    u8g2.begin();
    snakeSetup();
}

void loop() {
    snakeLoop();
}
*/
