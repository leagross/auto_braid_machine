// ============================================================================
//    Config.h
//    first_esp32.ino          — main logic
//    buttom_switch.ino        — start/emergency button
//    Ultrasonic_esp32.ino     — ultrasonic sensor
//    TCS34725_Color_sensor.ino — color sensor (I2C)
//    Dispenser.ino            — extension dispenser motor (moved here from the second board)
//
//  loop() is a single non-blocking poll: handleUart() and handleButton() run
//  every iteration no matter what else is happening — including while the
//  dispenser carousel is mid-turn — so the emergency button is never delayed
//  by a motor move. See Dispenser.ino for the non-blocking motor design.
// ============================================================================
#include "Config.h"

bool     armed              = false;  // whether we're in a braid session (monitoring for emergency)
bool     lastBtnPressed     = false;  // button state on the previous tick (to detect a new press)
uint32_t lastBtnChange      = 0;      // timestamp of the last press (for debounce)
String   rxLine             = "";     // buffer for the incoming UART line
bool     dispenserReplyPending = false;  // a DISP: request is in flight, waiting to send DISPDONE

void setup() {
  Serial.begin(115200);
  Serial2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  buttonSetup();
  ultrasonicSetup();
  colorSensorSetup();
  dispenserSetup();

  Serial.println("\n[FIRST] Sensors board ready");
}

void loop() {
  handleUart();                     // handles commands arriving from the second board
  handleButton();                   // monitors the emergency button
  if (dispenserReplyPending && dispenserTick()) {
    sendLine("DISPDONE");           // move finished -> report back now
    dispenserReplyPending = false;
  }
}

//  handleUart — reads a command from the second board
void handleUart() {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n' || c == '\r') {
      if (rxLine.length() > 0) {
        processCommand(rxLine);      // parse and act on it
        rxLine = "";
      }
    } else {
      rxLine += c;
      if (rxLine.length() > 40) rxLine = "";  // guard against garbage
    }
  }
}

//  processCommand — decides what to do based on the command
void processCommand(const String& cmd) {
  Serial.print("[FIRST] cmd: "); Serial.println(cmd);

  if (cmd == "DIST?") {
    float d = readDistanceCm();
    bool ok = (d >= DIST_MIN_CM && d <= DIST_MAX_CM);
    sendLine("DIST:" + String(d, 1));
    sendLine("DISTOK:" + String(ok ? 1 : 0));
  }
  else if (cmd == "COLOR?") {
    String name = readHairColor();
    sendLine("COLOR:" + name);
  }
  else if (cmd == "ARMED") {                   // braiding started
    armed = true; lastBtnPressed = false;
    Serial.println("[FIRST] ARMED");
  }
  else if (cmd == "DISARM") {                  // braiding finished
    armed = false;                             // stop monitoring for emergency
    Serial.println("[FIRST] DISARM");
  }
  else if (cmd == "PING") {                    // connectivity check
    sendLine("PONG");
  }
  else if (cmd.startsWith("DISP:")) {          // request to turn the extension carousel
    int idx = cmd.substring(5).toInt();
    dispenserStartMove(idx);                   // non-blocking — DISPDONE is sent once it finishes (see loop())
    dispenserReplyPending = true;
  }
}

//  handleButton — detects a new press and reports it based on current state
void handleButton() {
  bool pressed = buttonPressed();

  // new press (rising edge), past the debounce window
  if (pressed && !lastBtnPressed && (millis() - lastBtnChange > BTN_DEBOUNCE_MS)) {
    lastBtnChange = millis();
    if (armed) {                     // mid-braid
      sendLine("EMG");               // send emergency stop
      Serial.println("[FIRST] >>> EMERGENCY");
    } else {
      sendLine("OK");
      Serial.println("[FIRST] >>> OK/clear");
    }
  }
  lastBtnPressed = pressed;
}

//  sendLine — sends a text line to the second board
void sendLine(const String& s) {
  Serial2.print(s);
  Serial2.print('\n');
}
