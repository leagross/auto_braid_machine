# Function map — every function in the project, what it's responsible for, and the logic flow

Organized by board and file. The active system is **first_esp32 + second_esp32** (two boards,
UART), both driven as non-blocking state machines (see `second_esp32.ino`'s `SessionState` enum
and `first_esp32.ino`'s `loop()`).

---

## 🔵 first_esp32 — sensors board

### first_esp32.ino (main file)

| Function | Responsible for |
|---|---|
| `setup()` | initializes Serial (debug), Serial2 (UART to the second board), and every subsystem: button, ultrasonic, color, dispenser |
| `loop()` | the main loop — calls `handleUart()`, `handleButton()`, and the pending dispenser-tick check every iteration, never blocking |
| `handleUart()` | reads bytes from Serial2 byte by byte, assembles a line up to `\n`/`\r`, and passes each complete line to `processCommand()` |
| `processCommand(cmd)` | the protocol dispatcher — based on the received line (`DIST?`, `COLOR?`, `ARMED`, `DISARM`, `PING`, `DISP:<n>`) triggers the matching action and replies |
| `handleButton()` | reads the button via `buttonPressed()`, detects a new press (with debounce), and — based on `armed` — sends `EMG` (emergency) or `OK` (confirm) to the second board |
| `sendLine(s)` | sends a string + `\n` over Serial2 to the second board (the base send function) |

**Logic flow (`loop()`):**
1. `handleUart()` — reads whatever arrived from the second board; once a full line arrives (e.g.
   a `DIST?` request), `processCommand()` acts on it (e.g. reads the distance) and replies
   (`DIST:12.3` + `DISTOK:1`).
2. `handleButton()` — checks whether the physical button is pressed right now. If so, and
   `armed=true` (the system is "armed" to monitor for emergency — set by an `ARMED` received
   earlier) — sends `EMG`. If not armed (normal state) — sends `OK` (confirm/clear).
3. If a dispenser move finished this tick, sends `DISPDONE`.
4. Repeats forever with no `delay()` — so a UART byte or button press is never missed, and the
   button stays responsive even mid-move (see `Dispenser.ino`).

### buttom_switch.ino

| Function | Responsible for |
|---|---|
| `buttonSetup()` | configures the button pin as `INPUT_PULLUP` |
| `buttonPressed()` | reads the physical pin and returns whether the button is pressed right now (before debounce — debounce happens in `handleButton()`) |

### Ultrasonic_esp32.ino

| Function | Responsible for |
|---|---|
| `ultrasonicSetup()` | configures the HC-SR04's TRIG (output) and ECHO (input) pins |
| `readDistanceCm()` | sends a TRIG pulse, measures the echo time with `pulseIn()`, and computes distance in cm from the speed of sound (`duration * 0.0343 / 2.0`) |

**Logic flow:** `processCommand("DIST?")` calls `readDistanceCm()` once, compares it against
`DIST_MIN_CM..DIST_MAX_CM`, and replies with `DIST:<cm>` followed by `DISTOK:0/1`.

### TCS34725_Color_sensor.ino (I2C color sensor)

| Function | Responsible for |
|---|---|
| `colorSensorSetup()` | initializes I2C (`Wire.begin`) on the `COLOR_SDA`/`COLOR_SCL` pins and starts the sensor (`tcs.begin()`); logs an error if not found |
| `findClosestColor(r,g,b,c)` | compares the measured raw values (RAW R,G,B,Clear) against the `colorRefs` table (4 calibrated colors) using squared Euclidean distance in 4 dimensions; returns the closest color name, or `"Unknown"` if the minimum distance exceeds `MAX_COLOR_DIST` |
| `readHairColor()` | takes a **single** RAW sample (`tcs.getRawData`) and passes it to `findClosestColor()`; returns the color name as a `String` |

**Logic flow:** `processCommand("COLOR?")` calls `readHairColor()` once (a single RAW sample) and
replies with `COLOR:<name>`.
> Note: this works off RAW values (including the Clear channel), not normalized RGB — normalizing
> by Clear would erase exactly the difference between "black" (low Clear) and "blonde" (high Clear).

### Dispenser.ino (extension/carousel motor — ULN2003, unipolar)

Non-blocking: paced by `millis()` instead of `delay()`, so `handleButton()`/`handleUart()` keep
running every `loop()` iteration even while the carousel is turning.

| Function | Responsible for |
|---|---|
| `dispenserSetup()` | configures the 4 coil pins as OUTPUT and turns them off |
| `applyPhase(phase)` | energizes the coils per the phase table (`PHASE_SEQ`) for the current phase |
| `releaseCoils()` | turns off all 4 coils (saves heat when the motor isn't moving) |
| `stepOnce(dir)` | a single step — advances/retreats the phase, wrapping, and drives `applyPhase()` |
| `dispenserStartMove(targetIndex)` | computes how many steps and which direction are needed to reach the requested extension position (or "none"), and starts the (non-blocking) move |
| `dispenserTick()` | called every `loop()`; advances the move by at most one step, paced by `STEP_DELAY_MS`; returns `true` exactly once, the tick the move finishes |
| `dispenserIsMoving()` | whether a move is currently in progress |

**Logic flow (`PHASE_SEQ`):** each row in the table is a phase where 2 of the 4 coils are on (1)
and the rest off (0). Stepping through the rows in order (0→1→2→3→0...) creates a rotating
magnetic field that turns the rotor step by step; the direction (ascending/descending through the
table) sets forward/backward.

---

## 🟢 second_esp32 — master board (screen, motors, Firebase, HTTP server for the app)

### second_esp32.ino (main file)

The whole session is one explicit, non-blocking state machine (`enum SessionState` + a single
`switch` in `stateMachineTick()`), instead of the blocking sequential function calls this used to
be. Every state that used to wait (for a touch, a UART reply, or a motor to finish) now follows an
enter-once/poll-every-tick pattern.

| Function | Responsible for |
|---|---|
| `setup()` | initializes Serial, the screen (`displaySetup`), motors (`motorsSetup`), UART (`uartSetup`), Firebase (`firebaseSetup`), the HTTP server (`webServerSetup`), then kicks off the boot-time carousel homing |
| `loop()` | polls the UART link (`uartReceiveLines()`) and ticks the state machine (`stateMachineTick()`) — both non-blocking, every iteration |
| `stateMachineTick()` | **the central state machine of the whole project** — one `switch` over `SessionState`, one non-blocking step per call |
| `enterState(next)` | transitions to a new state and records when it was entered |
| `resetSessionAndReturnToWaitCode()` | clears the per-session variables and goes back to `ST_WAIT_CODE` |
| `findNextMyHairIndex(fromIdx)` | finds the next selected extension slot that's a "MyHair" (match my hair) choice, if any |
| `buildExtCsv()` | builds the comma-separated extension list to save with the order |
| `colorNameToIndex(name)` | converts a color name (from the color sensor) into the matching carousel position (for use with `uartRequestDispenserGoTo()`) |

**States, in flow order:**

1. `ST_BOOT_HOMING` — returns the carousel to position 0 at boot.
2. `ST_WAIT_CODE` → `ST_VALIDATING_CODE` → (`ST_CODE_REJECTED` and retry, or) `ST_WELCOME` — keypad
   entry, Firebase validation, brief confirmation.
3. `ST_SELECT_EXTENSIONS` — choosing up to `MAX_EXTENSIONS` extensions.
4. `ST_INSERT_HAIR` — waiting for START.
5. `ST_DIST_REQUEST` / `ST_DIST_WAIT` / `ST_DIST_BAD` — distance check loop against the first board.
6. `ST_COLOR_REQUEST` / `ST_COLOR_WAIT` / `ST_COLOR_SHOW` — resolves any "MyHair" selections via
   the color sensor, one at a time.
7. `ST_DISPENSE_NEXT` / `ST_DISPENSE_REQUEST` / `ST_TAKE_EXTENSION` / `ST_DISPENSE_HOME_REQUEST` —
   dispenses each chosen extension in turn, then returns the carousel home.
8. `ST_READY_TO_BRAID` — final explicit confirmation before the motors move.
9. `ST_BRAIDING` — `uartArm()` then non-blocking motor ticking (`braidTick()`) until finished or an
   emergency stop.
10. On emergency: `ST_EMERGENCY` (wait for clear) → `ST_RAIL_RETURNING` → `ST_SAVE_ORDER_EMERGENCY`
    (order saved with status `"emergency"`, code *not* released so the customer can retry) → back
    to `ST_WAIT_CODE`.
11. On success: `ST_DONE_SCREEN` → `ST_RAIL_RETURNING` → `ST_SAVE_ORDER_DONE` (status `"completed"`,
    code released) → `ST_SESSION_FINISHED` (brief "Saved!") → back to `ST_WAIT_CODE`.

### Motors.ino (rail and braid motors)

Non-blocking: `railTick()`/`braidTick()` advance at most one step per call, paced by `millis()`.
> **Known gap, not introduced by this refactor:** `braidTick()` only spins the braid motor — it
> does not lower the rail. `railSegmentsDone` is never incremented anywhere, so
> `railStartUpHome()` currently always computes 0 steps. The segmented-lowering design
> (`RAIL_SEGMENT_STEPS`/`RAIL_MAX_SEGMENTS` in `Config.h`) exists but nothing drives it yet.

| Function | Responsible for |
|---|---|
| `motorsSetup()` | configures the 8 pins of both motors (rail + braid) as OUTPUT, off |
| `applyPhase(p,phase)` | energizes coils per `PHASE_SEQ` (unipolar, ULN2003) — for the braid motor |
| `railApplyPhase(phase)` | energizes coils per `RAIL_PHASE_SEQ` (bipolar, L298N) — for the rail motor |
| `releaseCoils(p)` | turns off all coils of a given motor |
| `stepOnce(p,phase,dir)` | a single unipolar step (braid motor) in a given direction |
| `railStepOnce(dir)` | a single bipolar step (rail motor) in a given direction |
| `railStartMove(steps,dir)` | starts a non-blocking rail move of the given step count/direction |
| `railTick()` | called every `loop()`; advances the rail move by at most one step; returns `true` once, when it finishes |
| `railStartUpHome()` | starts moving the rail back up, by exactly the number of segments that were actually completed (`railSegmentsDone`) — so even after a mid-braid emergency stop, the return trip is accurate rather than always assuming the maximum distance |
| `braidStart()` | begins a non-blocking braid cycle (up to `BRAID_DURATION_MS`) |
| `braidTick()` | called every `loop()`; steps the braid motor, checks for emergency, tracks elapsed time; returns 0 while still going, 1 on normal finish, -1 on emergency stop |
| `motorsStopAll()` | emergency stop — immediately de-energizes both motors' coils |

### UartLink.ino (communication with the first board)

Async request/response: `uartRequestX()` sends a request and starts a timeout; `uartPollX()`,
polled every tick, returns 0 (still waiting), 1 (success), or -1 (timed out).

| Function | Responsible for |
|---|---|
| `uartSetup()` | initializes Serial2 (UART2, pins 16/17, 115200 baud) |
| `uartSend(s)` | sends a string + `\n` to the first board |
| `parseLine(line)` | parses a line received from the first board and updates the matching global (`EMG`, `OK`, `DIST:`, `DISTOK:`, `COLOR:`, `DISPDONE`) |
| `uartReceiveLines()` | reads whatever has accumulated in Serial2's buffer, assembles complete lines, and passes them to `parseLine()` — called every `loop()` iteration regardless of state, so nothing is ever missed |
| `uartRequestDistance()` / `uartPollDistance()` | sends `DIST?` / polls for the reply (checks `lastDistOk`) |
| `uartRequestHairColor()` / `uartPollHairColor()` | sends `COLOR?` / polls for the detected color (or "Unknown" on timeout) |
| `uartRequestDispenserGoTo(targetIndex)` / `uartPollDispenser()` | sends `DISP:<n>` / polls for the `DISPDONE` acknowledgment (or times out and continues anyway) |
| `uartArm()` | sends `ARMED` — starts emergency monitoring on the first board, resets the local flag |
| `uartDisarm()` | sends `DISARM` — stops emergency monitoring on the first board |

### DisplayManager.ino (TFT9341 touch screen)

Each screen follows an enter-once/poll-every-tick pattern instead of blocking. `touchEdge()` fires
once per new touch (rising edge only), replacing the old blocking wait-for-release spin loop.

| Function | Responsible for |
|---|---|
| `displaySetup()` | initializes the screen and touch controller |
| `touchEdge(buttonIdOut)` | shared helper — fires true exactly once per new touch, filling in which button was touched |
| `uiGetCodeEnter()` / `uiGetCodePoll(codeOut)` | draws the numeric keypad / polls it until a full code is confirmed |
| `drawExtButton(i,selected)` | draws a single button on the extension-choice screen, selected/unselected |
| `uiSelectExtensionsEnter(name)` / `uiSelectExtensionsPoll(tokensOut,countOut)` | draws the extension-choice screen / polls it until CONFIRM, returning which were chosen |
| `extName(token)` | converts an extension token (`EXT_BLONDE`/`EXT_GREEN`/`EXT_RED`/`EXT_BLACK`) to a display name |
| `uiOneButtonPoll()` | shared poll for any single-button screen |
| `uiInsertHairEnter()` | draws "insert hair" |
| `uiMessage(l1,l2)` | draws a general two-line status message |
| `uiTakeExtensionEnter(colorName)` | draws "take the extension" |
| `uiReadyToBraidEnter()` | draws "ready to braid?" before motion actually starts |
| `uiEmergencyEnter()` / `uiEmergencyPoll()` | draws the full-red emergency screen / polls until cleared, by either a touch or "OK" over UART |
| `uiDoneEnter()` | draws the full-green success screen |

### FirebaseManager.ino (WiFi + Cloud Firestore — codes/orders, not registration/login)

| Function | Responsible for |
|---|---|
| `fsStr(doc,field)` | extracts a text field (`stringValue`) from a Firestore document |
| `fsBool(doc,field)` | extracts a boolean field (`booleanValue`) from a Firestore document |
| `fsInt(doc,field)` | extracts a numeric field (`integerValue`) from a Firestore document |
| `firebaseSetup()` | connects to WiFi, signs into (or creates) the Firebase device account, and determines whether the system is in "real" or "demo" mode (`fbReady`) |
| `fbValidateCode(code,name,uid)` | checks against Firestore (or, in demo mode, just checks the length) whether the code exists and is unused, filling in name+uid |
| `fbReleaseCode(code)` | **deletes** the `codes/{code}` document entirely (not just marking `used=true`) — so the 4-digit number becomes free for reuse (only 9000 combinations exist) |
| `appendOrderJson(doc,docName,out)` | turns a single Firestore order document into simple JSON (`{id,uid,name,extensions,hairColor,status,createdAt}`) and appends it to the output |
| `fbListOrders(uidFilter)` | fetches all `orders` documents from Firestore (`listDocuments`); if `uidFilter` is non-empty, filters to that user's orders only (customer), otherwise returns everything (admin). Returns a ready-to-send `"[{...},{...}]"` JSON string |
| `fbDeleteOrder(orderId)` | deletes a single order document (`orders/{orderId}`) — used when a customer/admin removes an item from history |
| `fbSaveOrder(uid,name,extensions,hairColor,status)` | saves a new order document under `orders/{id}` with status "completed" or "emergency" |

### AuthManager.ino (register/login/temporary code — all Firebase Auth+Firestore traffic for the app)

| Function | Responsible for |
|---|---|
| `fsStrGet(doc,field)` | extracts a text field from a Firestore document (same as `fsStr` but with its own `FirebaseData` — `authFbdo` — so it doesn't collide with the main `fbdo` used by the braid flow) |
| `authRest(endpoint,email,pass,uidOut,errOut)` | calls the Firebase Auth REST API (`identitytoolkit.googleapis.com/v1/accounts:signUp` or `:signInWithPassword`) via `HTTPClient`; extracts `localId` (uid) from the response, or the error message |
| `authRegister(email,pass,name,uidOut,errOut)` | registration: creates an Auth account via `authRest("signUp",...)`, then creates a `users/{uid}` Firestore document with name/email/role="user" |
| `authLogin(email,pass,uidOut,nameOut,roleOut,errOut)` | login: authenticates via `authRest("signInWithPassword",...)`, then fetches name+role from the `users/{uid}` document (or defaults, if it doesn't exist) |
| `authGenerateCode(uid,name)` | generates a temporary code: rolls a 4-digit number, checks it's free in Firestore (up to 10 tries), and saves a `codes/{code}` document with uid/name/used=false |

### WebServer.ino (HTTP server — every app request goes through the ESP32, not directly to Firestore)

| Function | Responsible for |
|---|---|
| `sendJson(code,body)` | sends an HTTP response with a status code + JSON, including CORS headers (`Access-Control-Allow-*`) |
| `handleCors()` | handles `OPTIONS` (CORS preflight) requests — returns an empty `{}` |
| `bodyField(d,field)` | extracts a single field from the request body (JSON) sent by the app |
| `handleRegister()` | `POST /register` — reads email/password/name, calls `authRegister()`, returns `{uid,name,role}` |
| `handleLogin()` | `POST /login` — reads email/password, calls `authLogin()`, returns `{uid,name,role}` |
| `handleGenerateCode()` | `POST /generate-code` — reads uid/name, calls `authGenerateCode()`, returns `{code}` |
| `handleMyOrders()` | `GET /my-orders?uid=` — calls `fbListOrders(uid)`, returns that user's orders |
| `handleAllOrders()` | `GET /all-orders` — calls `fbListOrders("")` (no filter), returns every order (admin) |
| `handleDeleteOrder()` | `POST /delete-order` — reads orderId, calls `fbDeleteOrder()`, returns success/error |
| `webServerTask(param)` | the task loop running on **Core 0**, separate from the main state machine (`stateMachineTick()` on Core 1) — keeps calling `authServer.handleClient()` so the server always responds, even mid-braid |
| `webServerSetup()` | registers all the routes (POST/GET/OPTIONS above), starts the server, and runs `webServerTask` as a separate task via `xTaskCreatePinnedToCore` |

**Logic flow (a request from the app):** React App → `fetch()` to the ESP32's IP → arrives at
`webServerTask` (Core 0, always running in the background) → invokes the matching handler → calls
an `auth*`/`fb*` function → returns JSON → the app receives the response. This always works, even
if the board is mid-`stateMachineTick()` (braiding, etc.), because the two parts run on separate
cores.

---

## 🟣 react-app/src/api.js (the app's data layer)

| Function | Responsible for |
|---|---|
| `toSortMs(createdAt)` | converts `createdAt` (a `millis()` number from the ESP32) to a uniform number for sorting orders |
| `esp32Post(path,payload)` | sends a `POST` request to the ESP32 (with a JSON body), throws if `res.ok` is false |
| `esp32Get(path)` | sends a `GET` request to the ESP32 (with query params in the path), throws if `res.ok` is false |
| `api.register(email,pass,name)` | calls `esp32Post("/register",...)` |
| `api.login(email,pass)` | calls `esp32Post("/login",...)` |
| `api.logout()` | local cleanup only (there's no server-side session) |
| `api.generateCode(uid,name)` | calls `esp32Post("/generate-code",...)`, returns the code |
| `api.getMyOrders(uid)` | calls `esp32Get("/my-orders?uid=...")`, sorts by `createdAt` |
| `api.getAllOrders()` | calls `esp32Get("/all-orders")`, sorts by `createdAt` |
| `api.deleteOrder(orderId)` | calls `esp32Post("/delete-order",{orderId})` |

> **Note:** all 7 functions in `api` now go through the ESP32 (endpoints in `WebServer.ino`) —
> there is **no** direct Firestore access from the app anymore. `firebase.js` still initializes
> the Firebase SDK but nothing imports `auth`/`db` from it — it's unused, kept only for reference.
