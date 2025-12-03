// --- Global Variables (Unchanged) ---
String expression = "";
String result = "";

// The keyboard grid is slightly rearranged for better coverage and to fit '0'
const char keyboard[4][4] = { // Changed to 4x4 for a more standard grid
  {'7', '8', '9', '/'},
  {'4', '5', '6', '*'},
  {'1', '2', '3', '-'},
  {'0', '.', '=', 'C'}, // New row for 0, ., =, C (Clear)
};

const int rows = 4; // Updated row count
const int cols = 4; // Updated col count

int cursorRow = 0;
int cursorCol = 0;

// Debounce state placeholders (must be defined globally or passed)
static bool lastBtnUp = HIGH;
static bool lastBtnDown = HIGH;
static bool lastBtnLeft = HIGH;
static bool lastBtnRight = HIGH;
static bool lastBtnSelect = HIGH;
static bool lastBtnBack = HIGH;
static unsigned long upPressTime = 0;
static unsigned long downPressTime = 0;
static unsigned long leftPressTime = 0;
static unsigned long rightPressTime = 0;
static unsigned long selectPressTime = 0;
static unsigned long backPressTime = 0;
// ---------------------------------------------------------------------

// Assuming checkButtonPress utility is available
void claculaterloop(){
  
  // UP Navigation (Non-blocking)
  if (checkButtonPress(BTN_UP, lastBtnUp, upPressTime)) {
    moveCursor(-1, 0);
  }
  // DOWN Navigation (Non-blocking)
  if (checkButtonPress(BTN_DOWN, lastBtnDown, downPressTime)) {
    moveCursor(1, 0);
  }
  // LEFT Navigation (Non-blocking)
  if (checkButtonPress(BTN_LEFT, lastBtnLeft, leftPressTime)) {
    moveCursor(0, -1);
  }
  // RIGHT Navigation (Non-blocking)
  if (checkButtonPress(BTN_RIGHT, lastBtnRight, rightPressTime)) {
    moveCursor(0, 1);
  }
  // SELECT (Action) (Non-blocking)
  if (checkButtonPress(BTN_SELECT, lastBtnSelect, selectPressTime)) {
    handleSelect();
  }
  // BACK (Backspace/Delete) (Non-blocking)
  if (checkButtonPress(BTN_BACK, lastBtnBack, backPressTime)) {
    // Allows hold to delete multiple characters 
    if (expression.length() > 0) expression.remove(expression.length() - 1);
  }
  
  drawCalculatorUI();
}

void moveCursor(int dr, int dc) {
  // Cursor movement logic remains the same
  cursorRow = (cursorRow + dr + rows) % rows;
  cursorCol = (cursorCol + dc + cols) % cols;
}

void handleSelect() {
  char selected = keyboard[cursorRow][cursorCol];
  if (selected == 'C') {
    expression = "";
    result = "";
  } else if (selected == '=') {
    // NOTE: The evaluation logic is simple and may not handle complex expressions accurately.
    // For robust math, an external math parser library would be needed.
    result = String(evalExpression(expression), 2); // Print result with 2 decimal places
  } else {
    expression += selected;
  }
}

void drawCalculatorUI() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);

  // عرض المعادلة والنتيجة
  u8g2.setCursor(0, 10);
  u8g2.print("Expr: ");
  u8g2.print(expression);

  u8g2.setCursor(0, 20);
  u8g2.print("Res: ");
  u8g2.print(result);

  // أبعاد الأزرار (A 4x4 grid needs smaller boxes for the 128px screen)
  int boxW = 28; // 4 boxes * 28px = 112px
  int boxH = 10;
  int startX = 0;
  int startY = 24; 
  int spacingX = 4; // Total width 112 + 3*4 = 124px
  int spacingY = 4;

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      int x = startX + c * (boxW + spacingX);
      int y = startY + r * (boxH + spacingY);
      char ch = keyboard[r][c];

      if (r == cursorRow && c == cursorCol) {
        u8g2.drawBox(x, y, boxW, boxH);
        u8g2.setDrawColor(0); // Black text on white box
        u8g2.setCursor(x + (boxW/2) - 3, y + 8); // Centered text (approx)
        u8g2.print(ch);
        u8g2.setDrawColor(1); // Reset to white drawing color
      } else {
        u8g2.drawFrame(x, y, boxW, boxH);
        u8g2.setCursor(x + (boxW/2) - 3, y + 8);
        u8g2.print(ch);
      }
    }
  }

  // FIX: Removed the special '0' button drawing here.
  // The '0' key is now part of the main 4x4 grid at keyboard[3][0].

  u8g2.sendBuffer();
}

float evalExpression(String expr) {
  // Simple parser enhancement: convert to C-style string for easier tokenization
  char cstr[expr.length() + 1];
  expr.toCharArray(cstr, sizeof(cstr));

  // Tokenize by operators (+, -, *, /) and split into numbers and operators
  // This implementation is still not fully robust for complex order of operations,
  // but it's an improvement over the original single-pass sequential evaluation.
  
  float total = 0.0;
  char op = '+';
  String num = "";
  bool newNum = true;
  
  for (int i = 0; i <= expr.length(); i++) {
    char ch = (i == expr.length()) ? '\0' : expr[i];

    if (isdigit(ch) || ch == '.') {
      num += ch;
      newNum = false;
    } else if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '\0') {
      if (num.length() > 0) {
        float val = num.toFloat();
        
        // Basic sequential evaluation (same as original, but clearer)
        if (op == '+') total += val;
        else if (op == '-') total -= val;
        else if (op == '*') total *= val;
        else if (op == '/') {
          if (val != 0) total /= val;
          else return NAN; // Division by zero
        }
        // For the first number, just initialize total
        else if (newNum) total = val; 
      }
      
      if (ch != '\0') {
        op = ch;
        num = "";
      }
    }
  }
  
  return total;
}

