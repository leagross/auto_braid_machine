// ============================================================================
//  UartLink.ino — communication with the first board (sensors + emergency button)
//  See first_esp32.ino for the full protocol description.
//
//  Async request/response with the sensing board: only one request is ever in
//  flight at a time, matching how the state machine uses this (one call
//  site per request state). uartRequestX() sends the request and starts a
//  timer; uartPollX(), called every tick from the matching *_WAIT state,
//  returns 0 while waiting, 1 on success, -1 on timeout. uartReceiveLines()
//  itself is called once per loop() regardless of state, so the emergency/
//  clear flags stay fresh even outside of an active request.
// ============================================================================
#include "Config.h"

// Flags/values shared with other modules — defined here, read elsewhere
volatile bool emergencyRequested = false;  // "EMG" received
volatile bool clearRequested     = false;  // "OK" received
float  lastDistanceCm = -1;          // last distance reading
bool   lastDistOk     = false;       // whether the last distance was valid
String lastColor      = "Unknown";   // last detected color
volatile bool dispenserDone = false; // "DISPDONE" received (dispenser motor finished)

static String uRx = "";              // buffer for the incoming line
static uint32_t reqStartMs = 0;
static uint32_t reqTimeoutMs = 0;

// Init
void uartSetup() {
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
}

// Sends a text line to the first board
void uartSend(const String& s) {
  Serial2.print(s);
  Serial2.print('\n');
}

// Parses a received line and updates the relevant variable
static void parseLine(const String& line) {
  if (line == "EMG")                    emergencyRequested = true;   // emergency
  else if (line == "OK")                clearRequested = true;       // all clear
  else if (line.startsWith("DIST:"))    lastDistanceCm = line.substring(5).toFloat();
  else if (line.startsWith("DISTOK:"))  lastDistOk = (line.substring(7).toInt() == 1);
  else if (line.startsWith("COLOR:"))   lastColor = line.substring(6);
  else if (line == "DISPDONE")          dispenserDone = true;
}

// Reads everything available, assembles it into lines, updates flags. Called
// once per loop() tick regardless of state, so nothing is ever missed.
void uartReceiveLines() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n' || c == '\r') {
      if (uRx.length()) { parseLine(uRx); uRx = ""; }
    } else {
      uRx += c;
      if (uRx.length() > 40) uRx = "";  // guard against garbage
    }
  }
}

// ---- Distance ----
void uartRequestDistance() {
  Serial.println("[SECOND] Action: requesting distance from FIRST board...");
  lastDistanceCm = -1;
  uartSend("DIST?");
  reqStartMs = millis();
  reqTimeoutMs = 1500;
}

// 0 = still waiting, 1 = response received (check lastDistOk), -1 = timed out
int uartPollDistance() {
  if (lastDistanceCm >= 0) {
    Serial.printf("[SECOND] Result: distance=%.1fcm, OK=%s\n", lastDistanceCm, lastDistOk ? "YES" : "NO");
    return 1;
  }
  if (millis() - reqStartMs >= reqTimeoutMs) {
    Serial.println("[SECOND] Result: TIMEOUT waiting for distance");
    return -1;
  }
  return 0;
}

// ---- Hair color ----
void uartRequestHairColor() {
  Serial.println("[SECOND] Action: requesting hair color scan from FIRST board...");
  lastColor = "";
  uartSend("COLOR?");
  reqStartMs = millis();
  reqTimeoutMs = 4000;
}

// 0 = still waiting, 1 = color received (in lastColor), -1 = timed out
int uartPollHairColor() {
  if (lastColor.length()) {
    Serial.println("[SECOND] Result: detected color = " + lastColor);
    return 1;
  }
  if (millis() - reqStartMs >= reqTimeoutMs) {
    Serial.println("[SECOND] Result: TIMEOUT waiting for color");
    return -1;
  }
  return 0;
}

// ---- Extension dispenser ----
// targetIndex: 0..3 = color position, negative = EXT_NONE (first board just won't move).
void uartRequestDispenserGoTo(int targetIndex) {
  Serial.printf("[SECOND] Action: requesting dispenser move to %d...\n", targetIndex);
  dispenserDone = false;
  uartSend("DISP:" + String(targetIndex));
  reqStartMs = millis();
  reqTimeoutMs = 4000;
}

// 0 = still waiting, 1 = done, -1 = timed out (the state machine treats both
// the same way — "continue anyway" — matching the original behavior)
int uartPollDispenser() {
  if (dispenserDone) {
    Serial.println("[SECOND] Result: dispenser move DONE");
    return 1;
  }
  if (millis() - reqStartMs >= reqTimeoutMs) {
    Serial.println("[SECOND] Result: TIMEOUT waiting for dispenser (continuing anyway)");
    return -1;
  }
  return 0;
}

// Starts emergency monitoring on the first board (and clears the local flag)
void uartArm() {
  emergencyRequested = false;
  uartSend("ARMED");
}

// Stops emergency monitoring on the first board
void uartDisarm() {
  uartSend("DISARM");
}
