// ============================================================================
//  Motors.ino  —  מנועי הצעד של הלוח  (ESP32 שני)
// ============================================================================
#include "Config.h"

// פיני כל מנוע כמערך
static const int railPins[4]  = { RAIL_IN1,  RAIL_IN2,  RAIL_IN3,  RAIL_IN4  };
static const int braidPins[4] = { BRAID_IN1, BRAID_IN2, BRAID_IN3, BRAID_IN4 };

static int railPhase = 0;            // הפאזה הנוכחית של מנוע המסילה
static int braidPhase = 0;           // הפאזה הנוכחית של מנוע הקליעה

// כמה מקטעי ירידה (מתוך RAIL_MAX_SEGMENTS) הושלמו בפועל בקליעה הנוכחית.
// נשמר גם אם נעצרנו באמצע (חירום) — כך שבחזרה למעלה נדע בדיוק כמה להזיז,
static int railSegmentsDone = 0;

// רצף הפעלה דו-סלילי (מומנט גבוה) למנוע הקליעה — יוניפולרי, דרך ULN2003.
// כל פין הוא קצה סליל נפרד; "1,1,0,0" מדליק שני קצוות שכנים יחד.
static const uint8_t PHASE_SEQ[4][4] = {
  {1,1,0,0},                         // פאזה 0: סלילים 1+2
  {0,1,1,0},                         // פאזה 1: סלילים 2+3
  {0,0,1,1},                         // פאזה 2: סלילים 3+4
  {1,0,0,1}                          // פאזה 3: סלילים 4+1
};

// רצף הפעלה למנוע המסילה — ביפולרי (2 סלילים), דרך דרייבר L298N.
// IN1/IN2 שולטים בכיוון הזרם בסליל A, IN3/IN4 בסליל B (H-bridge לכל סליל).
// ⚠️ טבלה שונה לגמרי מ-PHASE_SEQ! "1,1" על אותו סליל ב-L298N = בלימה, לא הדלקה.
static const uint8_t RAIL_PHASE_SEQ[4][4] = {
  {1,0,1,0},                         // פאזה 0: A+ , B+
  {0,1,1,0},                         // פאזה 1: A- , B+
  {0,1,0,1},                         // פאזה 2: A- , B-
  {1,0,0,1}                          // פאזה 3: A+ , B-
};

// דגל החירום מוגדר ב-UartLink.ino;
extern volatile bool emergencyRequested;

//אתחול ראשי
void motorsSetup() {
  for (int i = 0; i < 4; i++) {      // מגדיר את 8 הפינים (2 מנועים) כיציאות ומכבה
    pinMode(railPins[i],  OUTPUT); digitalWrite(railPins[i],  LOW);
    pinMode(braidPins[i], OUTPUT); digitalWrite(braidPins[i], LOW);
  }
}

// מדליק את הסלילים לפי פאזה (יוניפולרי — מנוע הקליעה)
static void applyPhase(const int p[4], int phase) {
  for (int i = 0; i < 4; i++) digitalWrite(p[i], PHASE_SEQ[phase][i]);
}

// מדליק את הסלילים לפי פאזה (ביפולרי — מנוע המסילה, L298N)
static void railApplyPhase(int phase) {
  for (int i = 0; i < 4; i++) digitalWrite(railPins[i], RAIL_PHASE_SEQ[phase][i]);
}

// מכבה את כל הסלילים של מנוע)
static void releaseCoils(const int p[4]) {
  for (int i = 0; i < 4; i++) digitalWrite(p[i], LOW);
}

// פסיעה אחת של מנוע הקליעה (יוניפולרי) בכיוון dir (+1 קדימה / -1 אחורה)
static void stepOnce(const int p[4], int &phase, int dir) {
  phase = (phase + dir + 4) % 4;     // מקדם/מוריד פאזה במעגליות
  applyPhase(p, phase);              // מפעיל סלילים
  delay(STEP_DELAY_MS);             
}

// בכיוון dir (+1 קדימה / -1 אחורה)
static void railStepOnce(int dir) {
  railPhase = (railPhase + dir + 4) % 4;  // מקדם/מוריד פאזה במעגליות
  railApplyPhase(railPhase);              // מפעיל את הסלילים לפי הטבלה הביפולרית
  delay(STEP_DELAY_MS);                   
}

// ---------------------------------------------------------------------------
//  מסילה
// ---------------------------------------------------------------------------
// dir=+1 יורד למטה, dir=-1 עולה למעלה
void railMove(int steps, int dir) {
  Serial.printf("[SECOND] Action: rail moving %s (%d steps)...\n",
                dir > 0 ? "DOWN" : "UP", steps);
  for (int i = 0; i < steps; i++) {
    railStepOnce(dir);                  // תנועה פיזית — טבלת פאזות ביפולרית
  }
  releaseCoils(railPins);            // כיבוי בסיום)
  Serial.println("[SECOND] Result: rail movement done");
}

// מחזיר את המסילה למעלה — בדיוק לפי מספר המקטעים שבאמת הושלמו בירידה
void railUpHome() {
  int stepsToReverse = railSegmentsDone * RAIL_SEGMENT_STEPS;
  Serial.printf("[SECOND] Action: rail returning UP (%d segments = %d steps)\n",
                railSegmentsDone, stepsToReverse);
  if (stepsToReverse > 0) railMove(stepsToReverse, -1);
  railSegmentsDone = 0;
}

// ---------------------------------------------------------------------------
//  קליעה + ירידת מסילה יחד, במקטעים של
//  מחזיר true אם הסתיים תקין (כל המקטעים הושלמו), false אם נעצר בחירום.
// ---------------------------------------------------------------------------
bool braidWhileLowering() {
  Serial.println("[SECOND] Action: braiding started (spinning 1 minute, one direction)...");
  const int CHUNK = 8;                 // פסיעות בכל "מנה"
  const uint32_t BRAID_DURATION_MS = 60000;   // דקה אחת
  uint32_t startTime = millis();
  uint32_t lastProgressPrint = 0;

  while (millis() - startTime < BRAID_DURATION_MS) {
    uartReceiveLines();                 // בודק אם התקבל חירום
    if (emergencyRequested) {          // חירום!
      releaseCoils(braidPins);
      Serial.println("[SECOND] Result: EMERGENCY STOP during braiding");
      return false;                    // נעצרנו
    }
    for (int b = 0; b < CHUNK; b++) stepOnce(braidPins, braidPhase, +1);  // מסובב קליעה לכיוון אחד
    if (millis() - lastProgressPrint > 1000) {        // התקדמות כל שנייה בערך
      lastProgressPrint = millis();
      Serial.printf("[SECOND]   braiding: %lu/%lu ms\n",
                     (unsigned long)(millis() - startTime), (unsigned long)BRAID_DURATION_MS);
    }
  }

  releaseCoils(braidPins);
  Serial.println("[SECOND] Result: braiding finished successfully (1 minute reached)");
  return true;
}

// עצירת חירום — מכבה מיד את כל המנועים בלוח הזה
void motorsStopAll() {
  Serial.println("[SECOND] Action: emergency stop, releasing all motor coils");
  releaseCoils(railPins);
  releaseCoils(braidPins);
}
