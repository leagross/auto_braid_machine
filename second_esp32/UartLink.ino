// ============================================================================
//  UartLink.ino  —  תקשורת עם הלוח הראשון (חיישנים + כפתור חירום)
//  ראה תיאור הפרוטוקול המלא ב-first_esp32.ino
// ============================================================================
#include "Config.h"

// דגלים/ערכים משותפים — מוגדרים כאן, נקראים גם במודולים אחרים
volatile bool emergencyRequested = false;  // התקבל "EMG"
volatile bool clearRequested     = false;  // התקבל "OK"
float  lastDistanceCm = -1;          // המרחק האחרון
bool   lastDistOk     = false;       // האם המרחק תקין
String lastColor      = "Unknown";   // הצבע האחרון
volatile bool dispenserDone = false; // התקבל "DISPDONE" (מנוע התוספות סיים)

static String uRx = "";              // בופר לשורה נכנסת

//אתחול
void uartSetup() {
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
}

// שולח שורת טקסט ללוח הראשון
void uartSend(const String& s) {
  Serial2.print(s);
  Serial2.print('\n');
}

// מפענח שורה שהתקבלה ומעדכן משתנים
static void parseLine(const String& line) {
  if (line == "EMG")                    emergencyRequested = true;   // חירום
  else if (line == "OK")                clearRequested = true;       // הכל תקין
  else if (line.startsWith("DIST:"))    lastDistanceCm = line.substring(5).toFloat();
  else if (line.startsWith("DISTOK:"))  lastDistOk = (line.substring(7).toInt() == 1);
  else if (line.startsWith("COLOR:"))   lastColor = line.substring(6);
  else if (line == "DISPDONE")          dispenserDone = true;
}

// קורא את כל מה שהגיע, מרכיב לשורות ומעדכן דגלים (לקרוא לעיתים קרובות)
void uartReceiveLines() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n' || c == '\r') {
      if (uRx.length()) { parseLine(uRx); uRx = ""; }
    } else {
      uRx += c;
      if (uRx.length() > 40) uRx = "";  // הגנה מפני זבל
    }
  }
}

// מבקש מדידת מרחק וממתין לתשובה. מחזיר true אם המרחק תקין.
bool requestDistanceOk(uint32_t timeoutMs) {
  Serial.println("[SECOND] Action: requesting distance from FIRST board...");
  lastDistanceCm = -1;               // מאפס
  uartSend("DIST?");                 // שולח בקשה
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    uartReceiveLines();                      // קורא תשובות
    if (lastDistanceCm >= 0) {                   // התקבלה תשובה
      Serial.printf("[SECOND] Result: distance=%.1fcm, OK=%s\n",
                     lastDistanceCm, lastDistOk ? "YES" : "NO");
      return lastDistOk;
    }
  }
  Serial.println("[SECOND] Result: TIMEOUT waiting for distance");
  return false;                      // פסק זמן => לא תקין
}

// מבקש זיהוי צבע שיער וממתין לתשובה. מחזיר את שם הצבע (או "Unknown").
String requestHairColor(uint32_t timeoutMs) {
  Serial.println("[SECOND] Action: requesting hair color scan from FIRST board...");
  lastColor = "";                    // מאפס
  uartSend("COLOR?");                // שולח בקשה
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    uartReceiveLines();
    if (lastColor.length()) {                    // התקבל צבע
      Serial.println("[SECOND] Result: detected color = " + lastColor);
      return lastColor;
    }
  }
  Serial.println("[SECOND] Result: TIMEOUT waiting for color");
  return "Unknown";                  // פסק זמן
}

// מבקש מהלוח הראשון לסובב את קרוסלת התוספות ליעד הנתון, וממתין לסיום.
// targetIndex: 0..3 = מקום צבע, שלילי = EXT_NONE (הלוח הראשון פשוט לא זז).
void requestDispenserGoTo(int targetIndex, uint32_t timeoutMs) {
  Serial.printf("[SECOND] Action: requesting dispenser move to %d...\n", targetIndex);
  dispenserDone = false;              // מאפס
  uartSend("DISP:" + String(targetIndex));  // שולח בקשה
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    uartReceiveLines();
    if (dispenserDone) {                      // התקבל אישור סיום
      Serial.println("[SECOND] Result: dispenser move DONE");
      return;
    }
  }
  Serial.println("[SECOND] Result: TIMEOUT waiting for dispenser (continuing anyway)");
}

// מפעיל ניטור חירום בלוח הראשון (ומאפס את הדגל המקומי)
void uartArm() {
  emergencyRequested = false;
  uartSend("ARMED");
}

// מכבה ניטור חירום בלוח הראשון
void uartDisarm() {
  uartSend("DISARM");
}
