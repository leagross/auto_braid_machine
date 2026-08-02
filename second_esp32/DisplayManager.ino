// ============================================================================
//  DisplayManager.ino — touch screen (second ESP32)   <- "the screens"
//  ----------------------------------------------------------------------------
//  Each screen follows an enter/poll pattern instead of blocking:
//    uiXEnter()  — draws the screen once, when the state machine enters that state
//    uiXPoll()   — called once per loop() tick; returns true (with a result,
//                  where relevant) the moment the user's action is complete
//
//    uiGetCodeEnter()/uiGetCodePoll()             — numeric keypad for the temporary code
//    uiSelectExtensionsEnter()/uiSelectExtensionsPoll() — "Hi <name>" + choosing up to 3 extensions
//    uiInsertHairEnter()/uiOneButtonPoll()        — "insert hair" + START button
//    uiMessage()                                  — status message (no button, nothing to poll)
//    uiTakeExtensionEnter()/uiOneButtonPoll()      — "take the extension"
//    uiReadyToBraidEnter()/uiOneButtonPoll()       — "ready?" + START — right before braiding actually starts
//    uiEmergencyEnter()/uiEmergencyPoll()          — emergency screen — full red background
//    uiDoneEnter()/uiOneButtonPoll()               — braid-finished screen — full green background
//
//  Firebase connection: the code uiGetCodePoll() returns is validated by
//  fbValidateCode() (in FirebaseManager.ino, called from second_esp32.ino),
//  and the returned name is shown by uiSelectExtensionsEnter().
// ============================================================================
#include "Config.h"
#include "TFT9341Touch.h"

//                    cs, dc, tcs, tirq
tft9341touch lcd(TFT_CS, TFT_DC, TFT_TCS, TFT_TIRQ);  // screen object

extern volatile bool clearRequested;  // defined in UartLink.ino (emergency-clear from the physical button)

// Token and label for each extension-choice button
static const int   BTN_TOKENS[6] = { EXT_MYHAIR, EXT_GREEN, EXT_RED, EXT_BLONDE, EXT_BLACK, EXT_NONE };
static const char* BTN_LABELS[6] = { "MyHair", "Green", "Red", "Blonde", "Black", "None" };

// Init — called from the main setup()
void displaySetup() {
  lcd.begin();                       // screen init
  lcd.setTouch(3780, 372, 489, 3811);// touch calibration
  lcd.setRotation(3);                // 180° flipped from setRotation(1)
}

// Clears the screen and unregisters buttons from the previous screen
static void clearScreen() {
  lcd.fillScreen(BLACK);             // paint everything black
  lcd.clearButton();                 // clear previous buttons (so they stop responding to touch)
}

// ---------------------------------------------------------------------------
//  Shared touch edge-detector — replaces the old blocking waitRelease() spin
//  with the same rising-edge idiom already used for the physical button in
//  first_esp32.ino: fires once per new touch, not once per tick it stays down.
// ---------------------------------------------------------------------------
static bool touchWasDown = false;

// Returns true exactly once per new touch; on that call, buttonIdOut is set
// to the button under the touch (via lcd.ButtonTouch()).
static bool touchEdge(int &buttonIdOut) {
  bool down = lcd.touched();
  bool isNewPress = down && !touchWasDown;
  touchWasDown = down;
  if (!isNewPress) return false;
  delay(10);                          // brief settle before reading the touch controller
  lcd.readTouch();
  buttonIdOut = lcd.ButtonTouch(lcd.xTouch, lcd.yTouch);
  return true;
}

// ---------------------------------------------------------------------------
//  Screen: keypad — returns a CODE_LENGTH-digit code once confirmed
// ---------------------------------------------------------------------------
static String codeBuf = "";

static void drawCodeBuf() {
  String show = codeBuf;
  while ((int)show.length() < CODE_LENGTH) show += ' ';  // pad over old leftover digits
  lcd.print(30, 45, (char*)show.c_str(), 3, WHITE, BLACK);  // text box under the title
}

void uiGetCodeEnter() {
  clearScreen();
  lcd.print(10, 10, "ENTER CODE", 2, YELLOW, BLACK);  // title

  const int W = 70, H = 45, R = 5;   // button size (portrait)
  const int X[3] = { 10, 85, 160 };  // 3 columns (screen is 240 wide)
  const int Y[4] = { 95, 150, 205, 260 };  // 4 rows (screen is 320 tall)
  const char* lbl[12] = {"1","2","3","4","5","6","7","8","9","DEL","0","OK"};
  const uint16_t col[12] = {CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,RED,CYAN,GREEN};
  for (int i = 0; i < 12; i++)       // draws the 12 buttons (3x4)
    lcd.drawButton(i + 1, X[i % 3], Y[i / 3], W, H, R, col[i], BLACK, (char*)lbl[i], 3);

  codeBuf = "";
  drawCodeBuf();
}

// Returns true (with codeOut filled in) once OK is pressed with a full code
bool uiGetCodePoll(String& codeOut) {
  int b;
  if (!touchEdge(b)) return false;
  if (b >= 1 && b <= 9) { if ((int)codeBuf.length() < CODE_LENGTH) codeBuf += (char)('0' + b); }  // digit
  else if (b == 11)     { if ((int)codeBuf.length() < CODE_LENGTH) codeBuf += '0'; }              // 0
  else if (b == 10)     { if (codeBuf.length()) codeBuf.remove(codeBuf.length() - 1); }           // delete
  else if (b == 12 && (int)codeBuf.length() == CODE_LENGTH) { codeOut = codeBuf; return true; }    // confirm
  drawCodeBuf();
  return false;
}

// ---------------------------------------------------------------------------
//  Screen: welcome + extension selection — returns how many were chosen and fills tokensOut
// ---------------------------------------------------------------------------
static bool extSel[6];

static void drawExtButton(int i, bool selected) {
  const int W = 100, H = 45, R = 6;  // button size (portrait)
  const int X[2] = { 15, 125 };      // 2 columns (fits a 240-wide screen)
  const int Y[3] = { 80, 140, 200 }; // 3 rows
  uint16_t fill = selected ? YELLOW : BLUE;   // highlighted if selected
  uint16_t txt  = selected ? BLACK  : WHITE;
  lcd.drawButton(i + 1, X[i % 2], Y[i / 2], W, H, R, fill, txt, (char*)BTN_LABELS[i], 2);
}

void uiSelectExtensionsEnter(const String& name) {
  clearScreen();
  String hello = "Hi " + name;       // greeting with the name (came from Firebase)
  lcd.print(10, 10, (char*)hello.c_str(), 2, GREEN, BLACK);
  lcd.print(10, 45, "Choose (max 3):", 1, WHITE, BLACK);
  for (int i = 0; i < 6; i++) { extSel[i] = false; drawExtButton(i, false); }  // 6 choice buttons (2x3)
  lcd.drawButton(7, 125, 260, 100, 45, 6, GREEN, BLACK, (char*)"CONFIRM", 2);  // confirm
  lcd.drawButton(8, 15,  260, 100, 45, 6, ORANGE, BLACK, (char*)"CLEAR", 2);   // clear all
}

// Helper: counts how many extensions (excluding None) are currently selected
static int extSelCountNonNone() { int c = 0; for (int i = 0; i < 5; i++) if (extSel[i]) c++; return c; }

// Returns true (with tokensOut/countOut filled in) once CONFIRM is pressed
bool uiSelectExtensionsPoll(int tokensOut[MAX_EXTENSIONS], int& countOut) {
  int b;
  if (!touchEdge(b)) return false;

  if (b >= 1 && b <= 6) {            // choice button
    int idx = b - 1;
    if (idx == 5) {                  // "None" — clears everything else
      for (int i = 0; i < 5; i++) if (extSel[i]) { extSel[i] = false; drawExtButton(i, false); }
      extSel[5] = !extSel[5]; drawExtButton(5, extSel[5]);
    } else {
      if (!extSel[idx]) {            // selecting an extension
        if (extSel[5]) { extSel[5] = false; drawExtButton(5, false); }  // cancels None
        if (extSelCountNonNone() < MAX_EXTENSIONS) { extSel[idx] = true; drawExtButton(idx, true); }
      } else {                       // deselecting
        extSel[idx] = false; drawExtButton(idx, false);
      }
    }
  }
  else if (b == 7) {                 // CONFIRM
    int n = 0;
    for (int i = 0; i < 5; i++) if (extSel[i] && n < MAX_EXTENSIONS) tokensOut[n++] = BTN_TOKENS[i];
    countOut = n;                    // 0 => no extensions
    return true;
  }
  else if (b == 8) {                 // CLEAR — resets every selection at once
    for (int i = 0; i < 6; i++) if (extSel[i]) { extSel[i] = false; drawExtButton(i, false); }
  }
  return false;
}

// ---------------------------------------------------------------------------
//  Simple screens: message + one button (id 1)
// ---------------------------------------------------------------------------
static void oneButtonScreen(const char* l1, const char* l2, const char* btn, uint16_t btnColor) {
  clearScreen();
  lcd.print(10, 70, (char*)l1, 3, WHITE, BLACK);        // line 1
  if (l2 && l2[0]) lcd.print(10, 120, (char*)l2, 2, YELLOW, BLACK);  // line 2
  lcd.drawButton(1, 50, 210, 140, 55, 8, btnColor, BLACK, (char*)btn, 3);  // button (centered)
}

// A screen whose whole background is colored (e.g. full red for an emergency,
// full green for success) — not just the usual black background with a
// colored button. Used for states that need to stand out from a distance.
static void fullColorScreen(uint16_t bgColor, const char* l1, const char* l2,
                            const char* btn, uint16_t textColor) {
  lcd.fillScreen(bgColor);           // the whole screen (not just black) in the given color
  lcd.clearButton();                 // clear buttons from the previous screen
  lcd.print(10, 70, (char*)l1, 3, textColor, bgColor);            // line 1
  if (l2 && l2[0]) lcd.print(10, 120, (char*)l2, 2, textColor, bgColor);  // line 2
  // white button with black text — stands out against any colored background
  lcd.drawButton(1, 50, 210, 140, 55, 8, WHITE, BLACK, (char*)btn, 3);
}

// Shared poll for any single-button (id==1) screen
bool uiOneButtonPoll() {
  int b;
  if (!touchEdge(b)) return false;
  return (b == 1);
}

// "Insert hair" + START button
void uiInsertHairEnter() {
  oneButtonScreen("INSERT HAIR", "Press START", "START", GREEN);
}

// Status message (no button)
void uiMessage(const char* l1, const char* l2) {
  clearScreen();
  lcd.print(10, 90, (char*)l1, 3, WHITE, BLACK);
  if (l2 && l2[0]) lcd.print(10, 150, (char*)l2, 2, CYAN, BLACK);
}

// "Take the extension" + continue button
void uiTakeExtensionEnter(const char* colorName) {
  String s = "TAKE: "; s += colorName;
  oneButtonScreen((char*)s.c_str(), "Take from holder", "CONTINUE", CYAN);
}

// "Ready to braid?" + START — called after the distance check, color scan (if
// needed), and dispensing are all done. Braiding itself (rail+motor) doesn't
// start until this is pressed.
void uiReadyToBraidEnter() {
  oneButtonScreen("Ready!", "Press START to braid", "START", GREEN);
}

// Emergency screen — full red background (stands out from a distance).
// Cleared either by a touch on this screen, or by "OK" arriving over UART
// from the physical button on the first board (clearRequested).
void uiEmergencyEnter() {
  fullColorScreen(RED, "EMERGENCY", "Fix & press OK", "OK", WHITE);
  clearRequested = false;             // reset before polling (set in UartLink.ino)
}

bool uiEmergencyPoll() {
  if (clearRequested) return true;    // "OK" arrived from the physical button
  int b;
  if (touchEdge(b) && b == 1) return true;  // or a touch confirm on this screen
  return false;
}

// Braid-finished screen — full green background, shown once the motor
// reaches the end of the rail (braiding finished successfully, no
// emergency). Final confirmation button before the order gets saved.
void uiDoneEnter() {
  fullColorScreen(GREEN, "DONE :)", "Press FINISH", "FINISH", BLACK);
}

// Display name for a token
const char* extName(int token) {
  switch (token) {
    case EXT_BLONDE: return "Blonde";
    case EXT_GREEN:  return "Green";
    case EXT_RED:    return "Red";
    case EXT_BLACK:  return "Black";
    case EXT_MYHAIR: return "MyHair";
    default:         return "None";
  }
}
