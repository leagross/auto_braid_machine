
#include "Config.h"

static const int dispPins[4] = { DISP_IN1, DISP_IN2, DISP_IN3, DISP_IN4 };
static int dispPhase = 0;            // הפאזה הנוכחית של המנוע
static int dispIndex = 0;            // המיקום הנוכחי של הקרוסלה 

// רצף הפעלה דו-סלילי (מומנט גבוה) — 4 פאזות
static const uint8_t PHASE_SEQ[4][4] = {
  {1,1,0,0},                         // פאזה 0: סלילים 1+2
  {0,1,1,0},                         // פאזה 1: סלילים 2+3
  {0,0,1,1},                         // פאזה 2: סלילים 3+4
  {1,0,0,1}                          // פאזה 3: סלילים 4+1
};

// אתחול
void dispenserSetup() {
  for (int i = 0; i < 4; i++) {
    pinMode(dispPins[i], OUTPUT);
    digitalWrite(dispPins[i], LOW);
  }
}

// מדליק את הסלילים לפי פאזה
static void applyPhase(int phase) {
  for (int i = 0; i < 4; i++) digitalWrite(dispPins[i], PHASE_SEQ[phase][i]);
}

// מכבה את כל הסלילים
static void releaseCoils() {
  for (int i = 0; i < 4; i++) digitalWrite(dispPins[i], LOW);
}

// פסיעה אחת בכיוון dir (+1 קדימה / -1 אחורה)
static void stepOnce(int dir) {
  dispPhase = (dispPhase + dir + 4) % 4;  // מקדם/מוריד פאזה במעגליות
  applyPhase(dispPhase);
  delay(STEP_DELAY_MS);
}

// מסובב את הקרוסלה למקום התוספת לפי אינדקס הצבע (רבע סיבוב לכל מקום)
void dispenserGoTo(int targetIndex) {
  Serial.printf("[FIRST] Action: dispenser moving from %d to %d...\n", dispIndex, targetIndex);
  if (targetIndex < 0) {                 // EXT_NONE => לא זזים
    releaseCoils();
    Serial.println("[FIRST] Result: no target (EXT_NONE), staying in place");
    return;
  }
  int diff = targetIndex - dispIndex;    // כמה מקומות להזיז (יכול להיות שלילי)
  int dir  = (diff >= 0) ? 1 : -1;       // כיוון
  int moves = abs(diff);                 // מספר רבעי הסיבוב
  for (int m = 0; m < moves; m++)
    for (int s = 0; s < DISP_QUARTER_STEPS; s++)
      stepOnce(dir);
  releaseCoils();                        // כיבוי בסיום
  dispIndex = targetIndex;               // מעדכן מיקום
  Serial.printf("[FIRST] Result: dispenser reached position %d\n", dispIndex);
}
