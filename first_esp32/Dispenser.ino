// ============================================================================
//  Dispenser.ino — extension carousel motor (moved here from the second board)
//
//  Non-blocking: dispenserStartMove() kicks off a move, dispenserTick() steps
//  it forward by at most one motor step per call, paced by STEP_DELAY_MS via
//  millis() instead of delay(). Call dispenserTick() every loop() iteration —
//  this lets handleButton()/handleUart() keep running (in particular, the
//  emergency button) while the carousel is turning, instead of the board
//  being unresponsive for the ~1.5s a full rotation used to take.
// ============================================================================
#include "Config.h"

static const int dispPins[4] = { DISP_IN1, DISP_IN2, DISP_IN3, DISP_IN4 };
static int dispPhase = 0;            // current phase of the motor
static int dispIndex = 0;            // current carousel position
static int dispTargetIndex = 0;      // position the current move is heading to

static bool     dispMoving = false;
static int      dispStepsRemaining = 0;
static int      dispDir = 1;
static uint32_t dispLastStepMs = 0;

// Two-coil-on (high torque) drive sequence — 4 phases
static const uint8_t PHASE_SEQ[4][4] = {
  {1,1,0,0},                         // phase 0: coils 1+2
  {0,1,1,0},                         // phase 1: coils 2+3
  {0,0,1,1},                         // phase 2: coils 3+4
  {1,0,0,1}                          // phase 3: coils 4+1
};

// Init
void dispenserSetup() {
  for (int i = 0; i < 4; i++) {
    pinMode(dispPins[i], OUTPUT);
    digitalWrite(dispPins[i], LOW);
  }
}

// Energizes the coils for the given phase
static void applyPhase(int phase) {
  for (int i = 0; i < 4; i++) digitalWrite(dispPins[i], PHASE_SEQ[phase][i]);
}

// De-energizes all coils
static void releaseCoils() {
  for (int i = 0; i < 4; i++) digitalWrite(dispPins[i], LOW);
}

// Single step in direction dir (+1 forward / -1 backward)
static void stepOnce(int dir) {
  dispPhase = (dispPhase + dir + 4) % 4;  // advance/retreat phase, wrapping
  applyPhase(dispPhase);
}

// Starts turning the carousel to the given extension position (one quarter
// turn per position). Returns immediately — call dispenserTick() to advance it.
void dispenserStartMove(int targetIndex) {
  if (targetIndex < 0) {                 // EXT_NONE => stay in place
    releaseCoils();
    dispMoving = false;
    Serial.println("[FIRST] Result: no target (EXT_NONE), staying in place");
    return;
  }
  int diff = targetIndex - dispIndex;    // how many positions to move (can be negative)
  dispDir = (diff >= 0) ? 1 : -1;
  dispStepsRemaining = abs(diff) * DISP_QUARTER_STEPS;
  dispTargetIndex = targetIndex;
  dispLastStepMs = millis();
  dispMoving = (dispStepsRemaining > 0);
  Serial.printf("[FIRST] Action: dispenser moving from %d to %d...\n", dispIndex, targetIndex);
  if (!dispMoving) Serial.println("[FIRST] Result: already at target position");
}

// Call every loop() iteration. Takes at most one step, paced by STEP_DELAY_MS.
// Returns true exactly once, on the tick the move completes (false otherwise,
// including while idle).
bool dispenserTick() {
  if (!dispMoving) return false;
  if (millis() - dispLastStepMs < STEP_DELAY_MS) return false;

  stepOnce(dispDir);
  dispLastStepMs = millis();
  if (--dispStepsRemaining <= 0) {
    releaseCoils();
    dispIndex = dispTargetIndex;
    dispMoving = false;
    Serial.printf("[FIRST] Result: dispenser reached position %d\n", dispIndex);
    return true;
  }
  return false;
}

bool dispenserIsMoving() { return dispMoving; }
