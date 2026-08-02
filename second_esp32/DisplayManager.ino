// ============================================================================
//  DisplayManager.ino  —  מסך המגע  (ESP32 שני)   ← "המסכים"
//  ----------------------------------------------------------------------------
//  כאן נמצאים כל מסכי הממשק:
//    uiGetCode()          — מקלדת להקלדת הקוד הזמני
//    uiSelectExtensions() — "שלום <שם>" + בחירת עד 3 תוספות
//    uiInsertHair()       — "הכנס שיער" + כפתור START
//    uiMessage()          — הודעת סטטוס
//    uiTakeExtension()    — "קח את התוספת"
//    uiReadyToBraid()     — "מוכן?" + כפתור START — לפני שהקליעה מתחילה בפועל
//    uiEmergency()        — מסך חירום — רקע אדום מלא
//    uiDone()             — מסך סיום קליעה — רקע ירוק מלא
//
//  הקשר ל-Firebase: הקוד ש-uiGetCode() מחזיר מאומת ע"י fbValidateCode()
//  (בקובץ FirebaseManager.ino), והשם שחוזר מוצג ב-uiSelectExtensions().
//
// ============================================================================
#include "Config.h"
#include "TFT9341Touch.h"           

//                    cs, dc, tcs, tirq
tft9341touch lcd(TFT_CS, TFT_DC, TFT_TCS, TFT_TIRQ);  // אובייקט המסך

extern volatile bool clearRequested;  // מוגדר ב-UartLink.ino (אישור חירום מהכפתור)

// טוקן ותווית לכל כפתור בחירת תוספת
static const int   BTN_TOKENS[6] = { EXT_MYHAIR, EXT_GREEN, EXT_RED, EXT_BLONDE, EXT_BLACK, EXT_NONE };
static const char* BTN_LABELS[6] = { "MyHair", "Green", "Red", "Blonde", "Black", "None" };

// אתחול — נקרא מ-setup() הראשי
void displaySetup() {
  lcd.begin();                       // אתחול המסך
  lcd.setTouch(3780, 372, 489, 3811);// כיול המגע)
  lcd.setRotation(3);                // הפוך ב-180° מ-setRotation(1)
}

// מנקה מסך ומבטל רישום כפתורים ממסך קודם
static void clearScreen() {
  lcd.fillScreen(BLACK);             // צובע הכל בשחור
  lcd.clearButton();                 // מבטל כפתורים קודמים (שלא יגיבו למגע)
}

// ממתין עד שהאצבע מורמת (מונע לחיצה כפולה)
static void waitRelease() { while (lcd.touched()) ; }

// ---------------------------------------------------------------------------
//  מסך 1: מקלדת — מחזיר קוד בן CODE_LENGTH ספרות אחרי אישור
// ---------------------------------------------------------------------------
String uiGetCode() {
  clearScreen();
  lcd.print(10, 10, "ENTER CODE", 2, YELLOW, BLACK);  // כותרת

  const int W = 70, H = 45, R = 5;   // מידות כפתור (portrait)
  const int X[3] = { 10, 85, 160 };  // 3 עמודות (-240 )
  const int Y[4] = { 95, 150, 205, 260 };  // 4 שורות ( 320)
  const char* lbl[12] = {"1","2","3","4","5","6","7","8","9","DEL","0","OK"};
  const uint16_t col[12] = {CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,RED,CYAN,GREEN};
  for (int i = 0; i < 12; i++)       // מצייר את 12 הכפתורים (3x4)
    lcd.drawButton(i + 1, X[i % 3], Y[i / 3], W, H, R, col[i], BLACK, (char*)lbl[i], 3);

  String code = "";                  // הקוד שנצבר
  while (true) {
    if (lcd.touched()) {             // אם נגעו
      delay(10); lcd.readTouch();    // קורא נקודת מגע
      int b = lcd.ButtonTouch(lcd.xTouch, lcd.yTouch);  // איזה כפתור
      if (b >= 1 && b <= 9) { if ((int)code.length() < CODE_LENGTH) code += (char)('0' + b); }  // ספרה
      else if (b == 11)     { if ((int)code.length() < CODE_LENGTH) code += '0'; }              // 0
      else if (b == 10)     { if (code.length()) code.remove(code.length() - 1); }              // מחיקה
      else if (b == 12)     { if ((int)code.length() == CODE_LENGTH) { waitRelease(); return code; } }  // אישור

      String show = code;            // תצוגת הקוד
      while ((int)show.length() < CODE_LENGTH) show += ' ';  // ריפוד למחיקת ספרות ישנות
      lcd.print(30, 45, (char*)show.c_str(), 3, WHITE, BLACK);  // תיבת הטקסט מתחת לכותרת
      waitRelease();
    }
  }
}

//  מסך 2: ברוך הבא + בחירת תוספות — מחזיר כמה נבחרו וממלא tokensOut

static void drawExtButton(int i, bool selected) {
  const int W = 100, H = 45, R = 6;  // מידות כפתור (portrait)
  const int X[2] = { 15, 125 };      // 2 עמודות (מתאים ל-240 רוחב)
  const int Y[3] = { 80, 140, 200 }; // 3 שורות
  uint16_t fill = selected ? YELLOW : BLUE;   // מודגש אם נבחר
  uint16_t txt  = selected ? BLACK  : WHITE;
  lcd.drawButton(i + 1, X[i % 2], Y[i / 2], W, H, R, fill, txt, (char*)BTN_LABELS[i], 2);
}

int uiSelectExtensions(const String& name, int tokensOut[MAX_EXTENSIONS]) {
  clearScreen();
  String hello = "Hi " + name;       // ברכה עם השם (הגיע מ-Firebase)
  lcd.print(10, 10, (char*)hello.c_str(), 2, GREEN, BLACK);
  lcd.print(10, 45, "Choose (max 3):", 1, WHITE, BLACK);
  bool sel[6] = { false,false,false,false,false,false };  // מצב בחירה לכל כפתור
  for (int i = 0; i < 6; i++) drawExtButton(i, false);    // 6 כפתורי בחירה (2x3)
  lcd.drawButton(7, 125, 260, 100, 45, 6, GREEN, BLACK, (char*)"CONFIRM", 2);  // אישור
  lcd.drawButton(8, 15,  260, 100, 45, 6, ORANGE, BLACK, (char*)"CLEAR", 2);   // ניקוי הכל

  // עזר: סופר כמה תוספות (לא כולל None) נבחרו
  auto countNonNone = [&]() { int c = 0; for (int i = 0; i < 5; i++) if (sel[i]) c++; return c; };

  while (true) {
    if (lcd.touched()) {
      delay(10); lcd.readTouch();
      int b = lcd.ButtonTouch(lcd.xTouch, lcd.yTouch);
      if (b >= 1 && b <= 6) {        // כפתור בחירה
        int idx = b - 1;
        if (idx == 5) {              // "None" — מנקה את השאר
          for (int i = 0; i < 5; i++) if (sel[i]) { sel[i] = false; drawExtButton(i, false); }
          sel[5] = !sel[5]; drawExtButton(5, sel[5]);
        } else {
          if (!sel[idx]) {           // מסמן תוספת
            if (sel[5]) { sel[5] = false; drawExtButton(5, false); }  // מבטל None
            if (countNonNone() < MAX_EXTENSIONS) { sel[idx] = true; drawExtButton(idx, true); }
          } else {                   // מבטל סימון
            sel[idx] = false; drawExtButton(idx, false);
          }
        }
        waitRelease();
      }
      else if (b == 7) {             // CONFIRM
        waitRelease();
        int n = 0;
        for (int i = 0; i < 5; i++) if (sel[i] && n < MAX_EXTENSIONS) tokensOut[n++] = BTN_TOKENS[i];
        return n;                    // 0 => בלי תוספות
      }
      else if (b == 8) {             // CLEAR — מאפס את כל הבחירות בבת אחת
        for (int i = 0; i < 6; i++) if (sel[i]) { sel[i] = false; drawExtButton(i, false); }
        waitRelease();
      }
    }
  }
}

// ---------------------------------------------------------------------------
//  מסכים פשוטים: הודעה + כפתור אחד (id 1)
// ---------------------------------------------------------------------------
static void oneButtonScreen(const char* l1, const char* l2, const char* btn, uint16_t btnColor) {
  clearScreen();
  lcd.print(10, 70, (char*)l1, 3, WHITE, BLACK);        // שורה 1
  if (l2 && l2[0]) lcd.print(10, 120, (char*)l2, 2, YELLOW, BLACK);  // שורה 2
  lcd.drawButton(1, 50, 210, 140, 55, 8, btnColor, BLACK, (char*)btn, 3);  // כפתור (ממורכז)
}

// מסך שכל הרקע שלו צבוע (למשל אדום מלא לחירום, ירוק מלא לסיום מוצלח) — לא רק
// המסך השחור הרגיל עם כפתור צבעוני. משמש למצבים שצריך שיהיו בולטים מרחוק.
static void fullColorScreen(uint16_t bgColor, const char* l1, const char* l2,
                            const char* btn, uint16_t textColor) {
  lcd.fillScreen(bgColor);           // כל המסך (לא רק שחור) בצבע הרקע שהתקבל
  lcd.clearButton();                 // מבטל כפתורים ממסך קודם
  lcd.print(10, 70, (char*)l1, 3, textColor, bgColor);            // שורה 1
  if (l2 && l2[0]) lcd.print(10, 120, (char*)l2, 2, textColor, bgColor);  // שורה 2
  // כפתור לבן עם טקסט שחור — בולט על כל רקע צבעוני
  lcd.drawButton(1, 50, 210, 140, 55, 8, WHITE, BLACK, (char*)btn, 3);
}

// ממתין ללחיצה על הכפתור היחיד; ממשיך לקרוא UART (לא לפספס חירום/אישור)
static void waitOneButton() {
  while (true) {
    uartReceiveLines();                      // מ-UartLink.ino
    if (lcd.touched()) {
      delay(10); lcd.readTouch();
      if (lcd.ButtonTouch(lcd.xTouch, lcd.yTouch) == 1) { waitRelease(); return; }
    }
  }
}

// "הכנס שיער" + כפתור START
void uiInsertHair() {
  oneButtonScreen("INSERT HAIR", "Press START", "START", GREEN);
  waitOneButton();
}

// הודעת סטטוס (בלי כפתור)
void uiMessage(const char* l1, const char* l2) {
  clearScreen();
  lcd.print(10, 90, (char*)l1, 3, WHITE, BLACK);
  if (l2 && l2[0]) lcd.print(10, 150, (char*)l2, 2, CYAN, BLACK);
}

// "קח את התוספת" + כפתור המשך
void uiTakeExtension(const char* colorName) {
  String s = "TAKE: "; s += colorName;
  oneButtonScreen((char*)s.c_str(), "Take from holder", "CONTINUE", CYAN);
  waitOneButton();
}

// "מוכן לקליעה?" + כפתור START — נקרא אחרי שהמרחק נבדק, הצבע נסרק (אם צריך),
// והתוספות הוגשו. הקליעה בפועל (מסילה+מנוע) לא מתחילה עד שלוחצים כאן.
void uiReadyToBraid() {
  oneButtonScreen("Ready!", "Press START to braid", "START", GREEN);
  waitOneButton();
}

// מסך חירום — רקע אדום מלא (בולט מרחוק). מחזיר כשאושר "הכל תקין"
// (מסך המגע *או* הכפתור הפיזי בלוח הראשון, ששולח "OK" דרך ה-UART).
void uiEmergency() {
  fullColorScreen(RED, "EMERGENCY", "Fix & press OK", "OK", WHITE);
  clearRequested = false;             // מאפס לפני שממתינים (מ-UartLink.ino)
  while (true) {
    uartReceiveLines();                      // בודק אם הגיע "OK" מהכפתור הפיזי
    if (clearRequested) return;
    if (lcd.touched()) {             // או לחיצה במסך
      delay(10); lcd.readTouch();
      if (lcd.ButtonTouch(lcd.xTouch, lcd.yTouch) == 1) { waitRelease(); return; }
    }
  }
}

// מסך סיום קליעה — רקע ירוק מלא, מציג ברגע שהמנוע הגיע לסוף המסילה (קליעה
// הסתיימה בהצלחה, בלי חירום). כפתור אישור סופי לפני שמירת ההזמנה.
void uiDone() {
  fullColorScreen(GREEN, "DONE :)", "Press FINISH", "FINISH", BLACK);
  waitOneButton();
}

// שם צבע לתצוגה לפי טוקן
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
