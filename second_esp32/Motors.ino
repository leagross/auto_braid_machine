// ============================================================================
//  Motors.ino — this board's stepper motors (rail / braid)
//
//  Non-blocking: railTick()/braidTick() advance at most one motor step per
//  call, paced by STEP_DELAY_MS via millis() instead of delay(). Call them
//  every loop() iteration; they return a status the state machine checks to
//  know when to move on, instead of the old design where railMove()/
//  braidWhileLowering() blocked the whole board for seconds at a time.
//
//  NOTE (pre-existing, not something this refactor changed): braidTick() only
//  spins the braid motor — it does not lower the rail. railSegmentsDone is
//  never incremented anywhere, so railStartUpHome() currently always computes
//  0 steps to reverse. The segmented-lowering design (RAIL_SEGMENT_STEPS /
//  RAIL_MAX_SEGMENTS) exists in Config.h but nothing drives it yet. Flagged
//  here rather than silently "fixed", since it's not clear if this is
//  mid-development or intentional for now.
// ============================================================================
#include "Config.h"

// Pins for each motor, as arrays
static const int railPins[4]  = { RAIL_IN1,  RAIL_IN2,  RAIL_IN3,  RAIL_IN4  };
static const int braidPins[4] = { BRAID_IN1, BRAID_IN2, BRAID_IN3, BRAID_IN4 };

static int railPhase = 0;            // current phase of the rail motor
static int braidPhase = 0;           // current phase of the braid motor

// How many lowering segments (out of RAIL_MAX_SEGMENTS) were actually
// completed in the current braid. Kept even if stopped mid-way (emergency),
// so the return trip up knows exactly how far to go. See note above — always
// 0 today since nothing increments it yet.
static int railSegmentsDone = 0;

// Two-coil-on (high torque) drive sequence for the braid motor — unipolar,
// via ULN2003. Each pin is a separate coil end; "1,1,0,0" energizes two
// adjacent ends together.
static const uint8_t PHASE_SEQ[4][4] = {
  {1,1,0,0},                         // phase 0: coils 1+2
  {0,1,1,0},                         // phase 1: coils 2+3
  {0,0,1,1},                         // phase 2: coils 3+4
  {1,0,0,1}                          // phase 3: coils 4+1
};

// Drive sequence for the rail motor — bipolar (2 coils), via an L298N driver.
// IN1/IN2 control current direction in coil A, IN3/IN4 in coil B (an H-bridge
// per coil). Warning: a completely different table from PHASE_SEQ! "1,1" on
// the same coil on an L298N means braking, not "both on".
static const uint8_t RAIL_PHASE_SEQ[4][4] = {
  {1,0,1,0},                         // phase 0: A+ , B+
  {0,1,1,0},                         // phase 1: A- , B+
  {0,1,0,1},                         // phase 2: A- , B-
  {1,0,0,1}                          // phase 3: A+ , B-
};

// Emergency flag, defined in UartLink.ino
extern volatile bool emergencyRequested;

// Init
void motorsSetup() {
  for (int i = 0; i < 4; i++) {      // sets all 8 pins (2 motors) as outputs, off
    pinMode(railPins[i],  OUTPUT); digitalWrite(railPins[i],  LOW);
    pinMode(braidPins[i], OUTPUT); digitalWrite(braidPins[i], LOW);
  }
}

// Energizes the coils for a phase (unipolar — braid motor)
static void applyPhase(const int p[4], int phase) {
  for (int i = 0; i < 4; i++) digitalWrite(p[i], PHASE_SEQ[phase][i]);
}

// Energizes the coils for a phase (bipolar — rail motor, L298N)
static void railApplyPhase(int phase) {
  for (int i = 0; i < 4; i++) digitalWrite(railPins[i], RAIL_PHASE_SEQ[phase][i]);
}

// De-energizes all of a motor's coils
static void releaseCoils(const int p[4]) {
  for (int i = 0; i < 4; i++) digitalWrite(p[i], LOW);
}

// Single step of the braid motor (unipolar) in direction dir (+1 forward / -1 back)
static void stepOnce(const int p[4], int &phase, int dir) {
  phase = (phase + dir + 4) % 4;     // advance/retreat phase, wrapping
  applyPhase(p, phase);
}

// Single step of the rail motor, in direction dir (+1 forward / -1 back)
static void railStepOnce(int dir) {
  railPhase = (railPhase + dir + 4) % 4;  // advance/retreat phase, wrapping
  railApplyPhase(railPhase);              // drive the coils per the bipolar table
}

// ---------------------------------------------------------------------------
//  Rail — non-blocking, one step per elapsed STEP_DELAY_MS
// ---------------------------------------------------------------------------
static int      railStepsRemaining = 0;
static int      railDir = 1;
static uint32_t railLastStepMs = 0;
static bool     railMoving = false;

// dir=+1 moves down, dir=-1 moves up. Returns immediately — call railTick()
// every loop() to advance it.
static void railStartMove(int steps, int dir) {
  railStepsRemaining = steps;
  railDir = dir;
  railLastStepMs = millis();
  railMoving = (steps > 0);
  Serial.printf("[SECOND] Action: rail moving %s (%d steps)...\n",
                dir > 0 ? "DOWN" : "UP", steps);
}

// Call every loop() iteration. Returns true exactly once, on the tick the
// move finishes (or immediately if there was nothing to move).
bool railTick() {
  if (!railMoving) return true;
  if (millis() - railLastStepMs < STEP_DELAY_MS) return false;

  railStepOnce(railDir);
  railLastStepMs = millis();
  if (--railStepsRemaining <= 0) {
    releaseCoils(railPins);
    railMoving = false;
    Serial.println("[SECOND] Result: rail movement done");
    return true;
  }
  return false;
}

// Starts moving the rail back up — exactly as many steps as were actually
// completed going down, so a mid-braid emergency stop still returns home
// accurately instead of always assuming the maximum distance.
void railStartUpHome() {
  int stepsToReverse = railSegmentsDone * RAIL_SEGMENT_STEPS;
  railSegmentsDone = 0;
  railStartMove(stepsToReverse, -1);
}

// ---------------------------------------------------------------------------
//  Braiding — non-blocking, checks for emergency every tick
//  Call braidStart() once, then braidTick() every loop() until it returns
//  non-zero: 1 = finished normally, -1 = emergency stop.
// ---------------------------------------------------------------------------
static const uint32_t BRAID_DURATION_MS = 60000;   // one minute
static uint32_t braidStartMs = 0;
static uint32_t braidLastStepMs = 0;
static uint32_t braidLastPrintMs = 0;
static bool     braidActive = false;

void braidStart() {
  braidStartMs = braidLastStepMs = braidLastPrintMs = millis();
  braidActive = true;
  Serial.println("[SECOND] Action: braiding started (spinning 1 minute, one direction)...");
}

int braidTick() {
  if (!braidActive) return 1;

  if (emergencyRequested) {          // emergency!
    releaseCoils(braidPins);
    braidActive = false;
    Serial.println("[SECOND] Result: EMERGENCY STOP during braiding");
    return -1;                       // stopped
  }

  uint32_t now = millis();
  if (now - braidStartMs >= BRAID_DURATION_MS) {
    releaseCoils(braidPins);
    braidActive = false;
    Serial.println("[SECOND] Result: braiding finished successfully (1 minute reached)");
    return 1;
  }

  if (now - braidLastStepMs >= STEP_DELAY_MS) {
    stepOnce(braidPins, braidPhase, +1);   // spin the braid motor one direction
    braidLastStepMs = now;
  }
  if (now - braidLastPrintMs > 1000) {     // progress print, roughly once a second
    braidLastPrintMs = now;
    Serial.printf("[SECOND]   braiding: %lu/%lu ms\n",
                   (unsigned long)(now - braidStartMs), (unsigned long)BRAID_DURATION_MS);
  }
  return 0;                          // still going
}

// Emergency stop — immediately de-energizes both motors on this board
void motorsStopAll() {
  Serial.println("[SECOND] Action: emergency stop, releasing all motor coils");
  releaseCoils(railPins);
  releaseCoils(braidPins);
}
