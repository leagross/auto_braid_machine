# Project Analysis — AutoBraid

This is a documentation-only audit of the repository as it exists today. It does not propose code
changes; it exists to guide the documentation upgrade described in
[DOCUMENTATION_UPGRADE_REPORT.md](DOCUMENTATION_UPGRADE_REPORT.md) and to give a reviewer (or the
project owner) a single place to see what's strong, what's missing, and what to fix.

## 1. What already exists

AutoBraid is a physical hair-braiding machine controlled by two ESP32 boards over UART, with a
React + Firebase web app for customer-facing registration, booking codes, and order history. Before
this documentation pass, the repository already contained:

- **[README.md](../README.md)** — architecture diagram, wiring/pin tables, UART protocol table,
  Firestore schema, setup instructions, file-structure map. This is unusually complete for a student
  hardware project.
- **[components.md](../components.md)** — a bill of materials with quantities, roles, and notes
  (e.g. the ULN2003 vs. L298N phase-table warning).
- **[function_map.md](../function_map.md)** — every function in both firmware sketches and in
  `react-app/src/api.js`, with a description and per-file "logic flow" summary. This single file
  is close to a full technical spec of the firmware.
- **first_esp32/** and **second_esp32/** — two Arduino sketches, one file per subsystem/module
  (button, ultrasonic, color sensor, dispenser motor; display, motors, UART link, Firebase, auth,
  web server), each with a `Config.h` centralizing pins and constants.
- **react-app/** — a Vite + React app (Login/Register/Dashboard/Booking/MyAppointments/Admin pages),
  talking only to the second ESP32's HTTP API (`react-app/src/api.js`), with a unit test file
  (`api.test.js`) and a CI job that runs it.
- **.github/workflows/build.yml** — CI that compiles `first_esp32` (publicly available libraries
  only) and runs the React app's tests/build. `second_esp32` is explicitly excluded from CI because
  it depends on a display library (`TFT9341Touch`, by Avi Hayun) that isn't in the public Arduino
  Library Manager index — documented inline in the workflow.

## 2. Architecture (confirmed against source, not just README claims)

Two ESP32s, one master/one slave, connected by UART2 (115200 baud, pins 16/17 on both boards):

- **`first_esp32` (slave / "sensors board")** — owns the start/emergency button, the HC-SR04
  ultrasonic sensor, the TCS34725 color sensor, and (perhaps counter-intuitively) the extension
  **dispenser motor**, because the second board's pin budget is fully consumed by its soldered-on
  touchscreen. This is called out explicitly in the README and confirmed in `first_esp32.ino` /
  `Dispenser.ino`.
- **`second_esp32` (master / "brain")** — owns the ILI9341 touchscreen (via `TFT9341Touch`), the
  rail motor and braid motor, WiFi + Firebase, and an HTTP server (`WebServer.ino`) that is the
  *only* thing the React app ever talks to. Confirmed in `second_esp32.ino`'s `SessionState` enum
  (25 states) and `WebServer.ino`'s route handlers.
- Both boards are explicit, non-blocking state machines ticked once per `loop()` — confirmed by
  reading `stateMachineTick()` in `second_esp32.ino` and the poll/tick pattern in `Motors.ino`,
  `UartLink.ino`, `DisplayManager.ino`, and `Dispenser.ino`. There is no `delay()` anywhere in the
  hot path; this is a real engineering choice, not just a README claim, and is worth highlighting
  in the portfolio framing.
- The web server runs on FreeRTOS Core 0 (`xTaskCreatePinnedToCore` in `WebServer.ino`) while the
  session state machine runs on Core 1 — so the app stays responsive even mid-braid. This dual-core
  detail was not previously called out anywhere outside `function_map.md`.

## 3. Project strengths (worth foregrounding in the portfolio README)

- **Two independently-programmed microcontrollers coordinating over a hand-rolled ASCII UART
  protocol** — a real distributed-systems problem at small scale (who's the source of truth for the
  emergency button? what happens if a reply times out? — both are handled, see
  `uartPollDispenser()`'s timeout behavior).
- **Non-blocking state-machine firmware on both boards**, replacing an earlier blocking design —
  this is called out in-code as a deliberate refactor, and it's a strong signal of engineering
  maturity (recognizing and fixing a responsiveness bug in embedded control code).
- **Full-stack scope**: embedded C/C++, a REST-ish HTTP API hand-implemented on the ESP32, a React
  SPA, and Firebase Auth/Firestore as the system of record — end to end, one person's work.
- **Physical/mechanical scope**: stepper-driven rail and braid mechanisms, a rotating carousel
  dispenser, 3D-printed structural parts (not yet documented in the repo — see gaps below).
- **Honest in-code documentation of a known gap**: `Motors.ino` explicitly flags that the rail
  never actually lowers segment-by-segment (`railSegmentsDone` is never incremented), rather than
  silently leaving it broken or hiding it. This kind of transparency is rare and worth preserving
  in the new docs rather than smoothing over.
- **Working CI** for the parts that can be verified automatically (`first_esp32` compile, React
  test + build), with an honest comment explaining why `second_esp32` can't be included.

## 4. Missing documentation (addressed by this pass)

At the start of this documentation pass, the repository had **no `docs/` directory at all** and no
`hardware/` directory. Specifically missing:

- A dedicated hardware architecture document with a block diagram (README has a Mermaid diagram,
  but it's architecture-level, not a hardware signal-flow block diagram).
- State-machine diagrams — the `second_esp32` state machine (25 states) and the `first_esp32`
  loop/emergency logic exist only as an enum + switch in code and a prose list in the README; no
  visual diagram exists anywhere.
- A dedicated UART protocol document with a sequence diagram (the README has the command table but
  no sequence diagram of a normal exchange).
- Mechanical design documentation — no description of the rail mechanism, braiding mechanism,
  carousel dispenser, or any 3D-printed part, and no place in the repo to store CAD/STL files.
- A safety document distinguishing implemented behavior (button-triggered emergency stop, distance
  gating) from ideas that were never built (watchdog, timeouts, hardware cutoff).
- Any photos or a place to organize them.
- A single, standalone bill of materials file separate from the README (it existed as
  `components.md` at the repo root, not under `hardware/`).
- An interview-oriented project summary and an end-to-end system-flow document.

## 5. Documentation inconsistencies found

- `components.md` lives at the repository root; the task calls for `hardware/BOM.md`. Both will
  exist after this pass — `hardware/BOM.md` is the canonical BOM going forward, and
  `components.md` is kept (see the consistency audit in
  [DOCUMENTATION_UPGRADE_REPORT.md](DOCUMENTATION_UPGRADE_REPORT.md)) so existing links from README
  history aren't broken.
- `react-app/src/firebase.js` initializes the Firebase SDK client-side but is unused by any other
  file (confirmed: no other file imports `auth`/`db` from it) — both `README.md` and
  `function_map.md` already note this, and the new docs repeat it rather than contradict it.
- The React app has six pages (`LoginPage`, `RegisterPage`, `Dashboard`, `BookingPage`,
  `MyAppointmentsPage`, `AdminPage`); none of this page-level structure was previously documented
  outside of the file tree in the README.
- No naming inconsistency was found between "first board / FIRST" and "second board / SECOND" —
  the codebase, README, and function_map all agree on this terminology, and the new docs adopt it
  unchanged (rather than introducing "master ESP32" / "slave ESP32" as primary names, though those
  terms are used as clarifying synonyms per this task's own diagram wording).

## 6. Missing visual material

- No photos of the physical machine, electronics, or subassemblies exist in the repo.
- No demo-video documentation page existed (the link was in the README only).
- No CAD/STL files exist in the repo (mechanical design was apparently done outside version
  control, or not yet exported/organized).

## 7. Recommended presentation improvements (implemented in this pass)

- Add a `docs/` tree covering architecture, state machines, UART, mechanical design, safety,
  system flow, demo, images, and an interview-ready overview — cross-linked from a shorter, more
  scannable top-level README.
- Add a `hardware/` tree with `BOM.md` and a `mechanical/` folder (with `stl/` and `source/`
  subfolders) ready to receive the owner's existing CAD work.
- Keep every new document strictly derived from source code and existing docs — no invented
  specs, no invented URLs, no invented images.
