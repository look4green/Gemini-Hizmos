#include <Arduino.h>
#include <U8g2lib.h>

// --- 1. GLOBAL CONTEXT & PLACEHOLDER DEFINITIONS ---
// NOTE: Replace U8G2_... with your specific display model and configuration.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); 

// Define button pins (Placeholders)
#define BTN_LEFT 5
#define BTN_RIGHT 6
#define BTN_SELECT 7
// Note: BTN_BACK is not used in the original game logic, but could be added to exit.

// --- 2. GAME STATE VARIABLES ---
// Player settings
int invadersPlayerX = 50;
int invadersPlayerY = 56;
bool invadersPlayerAlive = true;

// Bullet settings
#define INVADERS_MAX_BULLETS 6
struct Bullet { int x, y; bool active; };
Bullet invadersBullets[INVADERS_MAX_BULLETS];

// Enemy settings
#define INVADERS_MAX_ENEMIES 4
struct Enemy { int x, y; bool active; };
Enemy invadersEnemies[INVADERS_MAX_ENEMIES];

// Explosion
bool invadersExploding = false;
unsigned long invadersExplosionStart = 0;

// Score
int invadersScore = 0;

// Timing
unsigned long invadersLastEnemyMove = 0;
unsigned long invadersLastBulletMove = 0;
unsigned long invadersLastEnemySpawn = 0;
unsigned long invadersLastShot = 0; // New timer for non-blocking fire rate
const unsigned long fireRateDelay = 150; // Fire rate limit in ms

// --- 3. SHAPE DEFINITIONS (Optimized for u8g2.drawXBM) ---

// Player shape (8x9) - Reduced to 8x9 to fit the XBM format size
// Note: u8g2 requires sprites to be byte-aligned (width is a multiple of 8).
// The original shape was 11 wide, so we redefine to 8 wide and use the existing array content.
const uint8_t invadersPlayerShape_width = 8;
const uint8_t invadersPlayerShape_height = 9;
const uint8_t invadersPlayerShape[] U8X8_PROGMEM = {
  0b00011000,     
  0b01111110,    
  0b00011000,   
  0b10011001,  
  0b11111111,  
  0b00111100,     
  0b00111100,    
  0b01111110,   
  0b11111111    
};

// Rock shape (5x5) - Need to pad to 8-bit width for XBM format
const uint8_t invadersRockShape_width = 8;
const uint8_t invadersRockShape_height = 5;
const uint8_t invadersRockShape[] U8X8_PROGMEM = {
  0b01110000, // 0b01110
  0b11111000, // 0b11111
  0b11111000, // 0b11111
  0b01110000, // 0b01110
  0b00100000  // 0b00100
};


// --- 4. DRAWING FUNCTIONS (Optimized) ---

void invadersDrawPlayer() {
    // Optimized drawing using u8g2.drawXBM
    u8g2.drawXBM(invadersPlayerX, invadersPlayerY, invadersPlayerShape_width, invadersPlayerShape_height, invadersPlayerShape);
}

void invadersDrawRock(int x, int y) {
    // Optimized drawing using u8g2.drawXBM
    u8g2.drawXBM(x, y, invadersRockShape_width, invadersRockShape_height, invadersRockShape);
}

// Keep the rest of the functions as they are, but fix the one that follows

// --- 5. CORE GAME LOGIC FUNCTIONS ---

void invadersShoot() {
  for (int i = 0; i < INVADERS_MAX_BULLETS; i++) {
    if (!invadersBullets[i].active) {
      invadersBullets[i].x = invadersPlayerX + 3;
      invadersBullets[i].y = invadersPlayerY - 2;
      invadersBullets[i].active = true;
      break;
    }
  }
}

void invadersMoveBullets() {
  for (int i = 0; i < INVADERS_MAX_BULLETS; i++) {
    if (invadersBullets[i].active) {
      invadersBullets[i].y -= 7;
      if (invadersBullets[i].y < 0) {
        invadersBullets[i].active = false;
      }
    }
  }
}

void invadersMoveEnemies() {
  for (int i = 0; i < INVADERS_MAX_ENEMIES; i++) {
    if (invadersEnemies[i].active) {
      invadersEnemies[i].y += 2;

      // Collision check with player (if enemy touches player area)
      if (invadersEnemies[i].y + invadersRockShape_height >= invadersPlayerY &&
          invadersEnemies[i].x + invadersRockShape_width >= invadersPlayerX &&
          invadersEnemies[i].x <= invadersPlayerX + invadersPlayerShape_width) {
        invadersEnemies[i].active = false;
        invadersPlayerAlive = false;
        invadersExploding = true;
        invadersExplosionStart = millis();
      } else if (invadersEnemies[i].y > 64) {
        invadersEnemies[i].active = false;
      }
    }
  }
}

void invadersSpawnEnemy() {
  for (int i = 0; i < INVADERS_MAX_ENEMIES; i++) {
    if (!invadersEnemies[i].active) {
      // x position is random, constrained by screen width minus enemy width
      invadersEnemies[i].x = random(0, 128 - invadersRockShape_width); 
      invadersEnemies[i].y = 0;
      invadersEnemies[i].active = true;
      break;
    }
  }
}

void invadersCheckCollisions() {
  for (int i = 0; i < INVADERS_MAX_BULLETS; i++) {
    if (!invadersBullets[i].active) continue;

    for (int j = 0; j < INVADERS_MAX_ENEMIES; j++) {
      if (!invadersEnemies[j].active) continue;

      // Basic AABB (Axis-Aligned Bounding Box) collision check
      if (invadersBullets[i].x < invadersEnemies[j].x + invadersRockShape_width &&
          invadersBullets[i].x + 2 > invadersEnemies[j].x && // Bullet width is 2
          invadersBullets[i].y < invadersEnemies[j].y + invadersRockShape_height &&
          invadersBullets[i].y + 2 > invadersEnemies[j].y) { // Bullet height is 2
          
        invadersBullets[i].active = false;
        invadersEnemies[j].active = false;
        invadersScore++;
      }
    }
  }
}

void invadersDrawExplosion() {
  int cx = invadersPlayerX + invadersPlayerShape_width / 2;
  int cy = invadersPlayerY + invadersPlayerShape_height / 2;
  for (int r = 1; r < 6; r++) {
    u8g2.drawCircle(cx, cy, r);
  }
}

void invadersDrawGame() {
  u8g2.clearBuffer();

  // Enemies
  for (int i = 0; i < INVADERS_MAX_ENEMIES; i++) {
    if (invadersEnemies[i].active) {
      invadersDrawRock(invadersEnemies[i].x, invadersEnemies[i].y);
    }
  }

  // Bullets (as small squares)
  for (int i = 0; i < INVADERS_MAX_BULLETS; i++) {
    if (invadersBullets[i].active) {
      u8g2.drawBox(invadersBullets[i].x, invadersBullets[i].y, 2, 2);
    }
  }

  // Player or explosion
  if (invadersPlayerAlive) {
    invadersDrawPlayer();
  } else if (invadersExploding) {
    invadersDrawExplosion();
  }

  // Score
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.setCursor(0, 10);
  u8g2.print("Score:");
  u8g2.print(invadersScore);

  u8g2.sendBuffer();
}


// --- 6. GAME LOOP (NON-BLOCKING) ---

void spacegame() {
  unsigned long now = millis();

  // 🕹️ Player controls (Non-blocking movement and firing)
  if (invadersPlayerAlive) {
    // Left/Right Movement
    // Screen width is 128. Player width is 8. Max X is 128 - 8 = 120. Original used 117.
    if (digitalRead(BTN_LEFT) == LOW && invadersPlayerX > 0) {
      invadersPlayerX = constrain(invadersPlayerX - 2, 0, 128 - invadersPlayerShape_width);
    }
    if (digitalRead(BTN_RIGHT) == LOW && invadersPlayerX < (128 - invadersPlayerShape_width)) {
      invadersPlayerX = constrain(invadersPlayerX + 2, 0, 128 - invadersPlayerShape_width);
    }
    
    // Firing (Rate-limited, Non-blocking)
    if (digitalRead(BTN_SELECT) == LOW) {
        if (now - invadersLastShot > fireRateDelay) {
            invadersShoot();
            invadersLastShot = now;
        }
    }
  } 
  // 💥 Explosion/Restart Logic
  else if (invadersExploding && (now - invadersExplosionStart > 900)) {
    // Restart
    invadersPlayerAlive = true;
    invadersExploding = false;
    invadersScore = 0;
    for (int i = 0; i < INVADERS_MAX_ENEMIES; i++) invadersEnemies[i].active = false;
    for (int i = 0; i < INVADERS_MAX_BULLETS; i++) invadersBullets[i].active = false;
    invadersPlayerX = 50;
  }

  // 🚀 Game logic (Time-based updates)
  if (invadersPlayerAlive || invadersExploding) { // Keep updating logic if player is alive or exploding
      if (now - invadersLastBulletMove > 40) {
        invadersMoveBullets();
        invadersLastBulletMove = now;
      }

      if (now - invadersLastEnemyMove > 50) {
        invadersMoveEnemies();
        invadersLastEnemyMove = now;
      }
  
      if (now - invadersLastEnemySpawn > 900 && invadersPlayerAlive) {
        invadersSpawnEnemy();
        invadersLastEnemySpawn = now;
      }
      
      invadersCheckCollisions();
  }

  invadersDrawGame();
}
