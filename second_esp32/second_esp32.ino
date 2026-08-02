// ============================================================================
//  second_esp32.ino  (Master)  —  main file
//  ----------------------------------------------------------------------------
//    Config.h            — pins and constants
//    second_esp32.ino    — setup/loop + the session state machine (this file)
//    DisplayManager.ino  — touch screen (keypad, extension choice, messages) <- "the screens"
//    Motors.ino          — this board's stepper motors (rail / braid)
//    UartLink.ino        — communication with the first board (sensors + emergency + dispenser)
//    FirebaseManager.ino — WiFi + Firebase (code validation + saving orders)
//
//  ARCHITECTURE: one explicit state machine, ticked once per loop(). Every
//  step that used to block — waiting for a touch, waiting for a UART reply,
//  turning a motor — is now "enter the state once, poll it every tick until
//  it's done", so loop() always returns quickly and nothing (the web server
//  on Core 0, the emergency button on the first board, a new UART line) is
//  ever kept waiting behind a single long call. See UartLink.ino/Motors.ino/
//  DisplayManager.ino for how each of those became non-blocking.
//
//  The one remaining exception is Firebase itself: fbValidateCode()/
//  fbSaveOrder()/fbReleaseCode() are still synchronous network calls from the
//  Firebase_ESP_Client library (see the note at the top of FirebaseManager.ino).
// ============================================================================
#include "Config.h"

// Updated by UartLink.ino's uartReceiveLines()/parseLine(), read directly here
// after a uartPollDistance()/uartPollHairColor() success (Arduino concatenates
// this file before UartLink.ino, so these need an explicit extern).
extern bool   lastDistOk;
extern String lastColor;

enum SessionState {
  ST_BOOT_HOMING,          // returning the dispenser to position 0 at boot
  ST_WAIT_CODE,            // keypad shown, waiting for a code to be entered
  ST_VALIDATING_CODE,      // entered code is being checked against Firebase
  ST_CODE_REJECTED,        // briefly shows "invalid code", then back to ST_WAIT_CODE
  ST_WELCOME,              // briefly shows "connected", then to ST_SELECT_EXTENSIONS
  ST_SELECT_EXTENSIONS,    // choosing up to 3 extensions
  ST_INSERT_HAIR,          // waiting for START after hair is inserted
  ST_DIST_REQUEST,         // sends DIST? to the first board
  ST_DIST_WAIT,            // waiting for the distance reply
  ST_DIST_BAD,             // briefly shows "bad distance", then retries
  ST_COLOR_REQUEST,        // sends COLOR? (only for a MyHair selection)
  ST_COLOR_WAIT,
  ST_COLOR_SHOW,           // briefly shows the detected color
  ST_DISPENSE_NEXT,        // advances to the next extension to dispense, or moves on
  ST_DISPENSE_REQUEST,     // sends DISP:<n> for the current extension
  ST_TAKE_EXTENSION,       // waiting for CONTINUE after taking the extension
  ST_DISPENSE_HOME_REQUEST,// returns the carousel to position 0 once all extensions are out
  ST_READY_TO_BRAID,       // waiting for START right before the motors move
  ST_BRAIDING,             // non-blocking motor stepping, up to 1 minute or an emergency
  ST_EMERGENCY,            // red screen, waiting for it to be cleared
  ST_DONE_SCREEN,          // green "DONE" screen, waiting for FINISH
  ST_RAIL_RETURNING,       // non-blocking rail-up movement (after either DONE or EMERGENCY)
  ST_SAVE_ORDER_DONE,      // saves the order as "completed", shows "Saved!"
  ST_SESSION_FINISHED,     // brief "Saved!" message, then back to ST_WAIT_CODE
  ST_SAVE_ORDER_EMERGENCY, // saves the order as "emergency", then straight back to ST_WAIT_CODE
};

static SessionState state = ST_BOOT_HOMING;
static uint32_t stateEnteredAt = 0;

// Per-session data (replaces what used to be locals inside runSession())
static String sName, sUid, sCode;
static int    sTokens[MAX_EXTENSIONS];
static int    sExtCount = 0;
static String sHairColor = "-";
static int    sColorScanIdx = -1;   // which sTokens[] index is currently being MyHair-resolved
static int    sDispenseIdx = -1;    // which sTokens[] index is currently being dispensed
static String sExtCsv;
static bool   sSessionOk = true;    // whether the braid finished normally vs. an emergency

static void enterState(SessionState next) {
  state = next;
  stateEnteredAt = millis();
}

static void resetSessionAndReturnToWaitCode() {
  sName = ""; sUid = ""; sCode = "";
  sExtCount = 0; sHairColor = "-"; sColorScanIdx = -1; sDispenseIdx = -1;
  sExtCsv = "";
  enterState(ST_WAIT_CODE);
}

// Scans sTokens[0..sExtCount) for the next EXT_MYHAIR entry after fromIdx (-1 to start from the beginning)
static int findNextMyHairIndex(int fromIdx) {
  for (int i = fromIdx + 1; i < sExtCount; i++) if (sTokens[i] == EXT_MYHAIR) return i;
  return -1;
}

// Builds the CSV list of extensions to save — needed even if an emergency happens later
static void buildExtCsv() {
  sExtCsv = "";
  for (int i = 0; i < sExtCount; i++) {
    if (sTokens[i] == EXT_NONE) continue;
    if (sExtCsv.length()) sExtCsv += ",";
    sExtCsv += extName(sTokens[i]);
  }
  if (sExtCsv.length() == 0) sExtCsv = "None";
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n[SECOND] Master starting");

  displaySetup();
  motorsSetup();
  uartSetup();

  uiMessage("Starting...", "Please wait");
  firebaseSetup();
  webServerSetup();

  uartRequestDispenserGoTo(0);       // home the carousel — polled in ST_BOOT_HOMING below
  enterState(ST_BOOT_HOMING);
}

void loop() {
  uartReceiveLines();                // always poll the link to the first board, regardless of state
  stateMachineTick();
}

// ===========================================================================
//  stateMachineTick — one non-blocking step of whatever the current session is doing
// ===========================================================================
void stateMachineTick() {
  static SessionState lastState = (SessionState)-1;
  bool entering = (state != lastState);
  lastState = state;

  switch (state) {

    case ST_BOOT_HOMING: {
      if (uartPollDispenser() != 0) {
        Serial.println("========== [SECOND] READY ==========\n");
        enterState(ST_WAIT_CODE);
      }
      break;
    }

    // ---- 1+2: code entry + Firebase validation ----
    case ST_WAIT_CODE: {
      if (entering) {
        Serial.println("\n========== [SECOND] NEW SESSION ==========");
        Serial.println("[SECOND] Stage 1: waiting for code entry...");
        uiGetCodeEnter();
      }
      String code;
      if (uiGetCodePoll(code)) {
        sCode = code;
        Serial.println("[SECOND] Action: code entered = " + sCode);
        enterState(ST_VALIDATING_CODE);
      }
      break;
    }
    case ST_VALIDATING_CODE: {
      if (entering) uiMessage("Checking...", "");
      if (fbValidateCode(sCode, sName, sUid)) {
        Serial.println("[SECOND] Result: logged in as " + sName + " (uid=" + sUid + ")");
        enterState(ST_WELCOME);
      } else {
        Serial.println("[SECOND] Result: code REJECTED, asking again");
        enterState(ST_CODE_REJECTED);
      }
      break;
    }
    case ST_CODE_REJECTED: {
      if (entering) uiMessage("Invalid code", "Try again");
      if (millis() - stateEnteredAt > 1500) enterState(ST_WAIT_CODE);
      break;
    }
    case ST_WELCOME: {
      if (entering) uiMessage("Connected!", "Code verified in database");
      if (millis() - stateEnteredAt > 1200) {
        Serial.println("[SECOND] Stage 2: waiting for extension selection...");
        enterState(ST_SELECT_EXTENSIONS);
      }
      break;
    }

    // ---- 3: extension selection ----
    case ST_SELECT_EXTENSIONS: {
      if (entering) uiSelectExtensionsEnter(sName);
      int count;
      if (uiSelectExtensionsPoll(sTokens, count)) {
        sExtCount = count;
        Serial.printf("[SECOND] Result: %d extension(s) selected\n", count);
        Serial.println("[SECOND] Stage 3: waiting for hair insertion + START...");
        enterState(ST_INSERT_HAIR);
      }
      break;
    }

    // ---- 4: insert hair ----
    case ST_INSERT_HAIR: {
      if (entering) uiInsertHairEnter();
      if (uiOneButtonPoll()) {
        Serial.println("[SECOND] Stage 4: distance check...");
        enterState(ST_DIST_REQUEST);
      }
      break;
    }

    // ---- 5: distance check (ultrasonic sensor on the first board) ----
    case ST_DIST_REQUEST: {
      if (entering) { uiMessage("Checking", "distance..."); uartRequestDistance(); }
      enterState(ST_DIST_WAIT);
      break;
    }
    case ST_DIST_WAIT: {
      int r = uartPollDistance();
      if (r == 0) break;                          // still waiting
      if (r == 1 && lastDistOk) {
        sColorScanIdx = findNextMyHairIndex(-1);
        if (sColorScanIdx >= 0) {
          Serial.println("[SECOND] Stage 5: MyHair selected -> scanning color...");
          enterState(ST_COLOR_REQUEST);
        } else {
          enterState(ST_DISPENSE_NEXT);
        }
      } else {
        enterState(ST_DIST_BAD);                  // bad reading or timeout -> retry
      }
      break;
    }
    case ST_DIST_BAD: {
      if (entering) uiMessage("Bad distance", "Please adjust");
      if (millis() - stateEnteredAt > 1500) enterState(ST_DIST_REQUEST);
      break;
    }

    // ---- 5b: resolving "match my hair" via the color sensor ----
    case ST_COLOR_REQUEST: {
      if (entering) { uiMessage("Scanning", "hair color..."); uartRequestHairColor(); }
      enterState(ST_COLOR_WAIT);
      break;
    }
    case ST_COLOR_WAIT: {
      int r = uartPollHairColor();
      if (r == 0) break;
      sHairColor = (r == 1) ? lastColor : "Unknown";
      sTokens[sColorScanIdx] = colorNameToIndex(sHairColor);  // turns it into a carousel position
      Serial.println("[SECOND] Result: MyHair resolved to -> " + sHairColor);
      enterState(ST_COLOR_SHOW);
      break;
    }
    case ST_COLOR_SHOW: {
      if (entering) uiMessage("Detected:", sHairColor.c_str());
      if (millis() - stateEnteredAt > 1800) {
        int next = findNextMyHairIndex(sColorScanIdx);
        sColorScanIdx = next;
        enterState(next >= 0 ? ST_COLOR_REQUEST : ST_DISPENSE_NEXT);
      }
      break;
    }

    // ---- 6: dispensing extensions (one quarter turn per color) ----
    case ST_DISPENSE_NEXT: {
      if (entering) Serial.println("[SECOND] Stage 6: dispensing extensions...");
      int i = (sDispenseIdx < 0) ? 0 : sDispenseIdx + 1;
      while (i < sExtCount && sTokens[i] == EXT_NONE) i++;
      sDispenseIdx = i;
      if (i < sExtCount) {
        enterState(ST_DISPENSE_REQUEST);
      } else {
        enterState(ST_DISPENSE_HOME_REQUEST);
      }
      break;
    }
    case ST_DISPENSE_REQUEST: {
      if (entering) {
        Serial.println("[SECOND] Action: dispensing " + String(extName(sTokens[sDispenseIdx])));
        uartRequestDispenserGoTo(sTokens[sDispenseIdx]);
      }
      if (uartPollDispenser() != 0) enterState(ST_TAKE_EXTENSION);
      break;
    }
    case ST_TAKE_EXTENSION: {
      if (entering) uiTakeExtensionEnter(extName(sTokens[sDispenseIdx]));
      if (uiOneButtonPoll()) enterState(ST_DISPENSE_NEXT);
      break;
    }
    case ST_DISPENSE_HOME_REQUEST: {
      if (entering) uartRequestDispenserGoTo(0);   // return the carousel to its starting position
      if (uartPollDispenser() != 0) {
        buildExtCsv();                             // needed even if an emergency happens later
        Serial.println("[SECOND] Stage 6b: waiting for braid START confirmation...");
        enterState(ST_READY_TO_BRAID);
      }
      break;
    }

    // ---- 6b: explicit confirmation before the motors move ----
    case ST_READY_TO_BRAID: {
      if (entering) uiReadyToBraidEnter();
      if (uiOneButtonPoll()) {
        Serial.println("[SECOND] Stage 7: braiding...");
        uiMessage("Braiding...", "Press btn = stop");
        uartArm();                 // tells the first board to start monitoring for emergency
        braidStart();
        enterState(ST_BRAIDING);
      }
      break;
    }

    // ---- 7: braiding — rail down + braid motor turning ----
    case ST_BRAIDING: {
      int r = braidTick();         // 0 = still going, 1 = finished ok, -1 = emergency stop
      if (r == 0) break;
      uartDisarm();                // tells the first board to stop monitoring
      if (r == 1) {
        sSessionOk = true;
        Serial.println("[SECOND] Stage 9: braid complete (motor reached end of rail)");
        enterState(ST_DONE_SCREEN);
      } else {
        sSessionOk = false;
        Serial.println("[SECOND] Stage 8: EMERGENCY handling...");
        motorsStopAll();
        enterState(ST_EMERGENCY);
      }
      break;
    }

    // ---- 8: emergency handling ----
    case ST_EMERGENCY: {
      if (entering) uiEmergencyEnter();
      if (uiEmergencyPoll()) {
        Serial.println("[SECOND] Action: emergency cleared, resetting...");
        uiMessage("Resetting...", "");
        railStartUpHome();
        enterState(ST_RAIL_RETURNING);
      }
      break;
    }

    // ---- 9: normal finish ----
    case ST_DONE_SCREEN: {
      if (entering) uiDoneEnter();
      if (uiOneButtonPoll()) {
        Serial.println("[SECOND] Stage 9b: finishing + saving order...");
        uiMessage("Finishing...", "");
        railStartUpHome();
        enterState(ST_RAIL_RETURNING);
      }
      break;
    }

    // ---- shared tail: rail returns home, then save the order ----
    case ST_RAIL_RETURNING: {
      if (railTick()) {
        enterState(sSessionOk ? ST_SAVE_ORDER_DONE : ST_SAVE_ORDER_EMERGENCY);
      }
      break;
    }
    case ST_SAVE_ORDER_DONE: {
      fbSaveOrder(sUid, sName, sExtCsv, sHairColor, "completed");
      fbReleaseCode(sCode);        // deletes the code -> free for reuse
      uiMessage("Saved!", sName.c_str());
      enterState(ST_SESSION_FINISHED);
      break;
    }
    case ST_SESSION_FINISHED: {
      if (millis() - stateEnteredAt > 2500) {
        Serial.println("========== [SECOND] SESSION DONE ==========\n");
        resetSessionAndReturnToWaitCode();
      }
      break;
    }
    case ST_SAVE_ORDER_EMERGENCY: {
      // Saved on a separate document from a normal "completed" order. The
      // code is *not* marked used, so the customer can try again.
      fbSaveOrder(sUid, sName, sExtCsv, sHairColor, "emergency");
      Serial.println("========== [SECOND] SESSION ABORTED (emergency) ==========\n");
      resetSessionAndReturnToWaitCode();
      break;
    }
  }
}

// ===========================================================================
//  colorNameToIndex — converts a color name (from the sensor) to a carousel position
// ===========================================================================
int colorNameToIndex(const String& name) {
  if (name == "Blonde") return EXT_BLONDE;
  if (name == "Green")  return EXT_GREEN;
  if (name == "Red")    return EXT_RED;
  if (name == "Black")  return EXT_BLACK;
  return EXT_NONE;
}
