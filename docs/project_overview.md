# Project Overview — AutoBraid

A concise, technical, interview-ready summary. For depth, follow the links to the dedicated docs.

## 1. What AutoBraid does

A physical machine that braids hair extensions into a user's hair automatically. A customer
registers and logs into a React web app, generates a one-time 4-digit code, walks up to the
machine, enters the code on its touchscreen, chooses up to 3 extension colors (or "match my hair"),
inserts their hair, and the machine dispenses the extensions and runs the braiding mechanism —
with a physical emergency-stop button available throughout. See [system_flow.md](system_flow.md).

## 2. Main technical challenge

Coordinating real-time, safety-relevant hardware control (motors, an emergency button, sensors)
with a UI, cloud auth/data, and an HTTP API — all on resource-constrained microcontrollers — without
any single slow operation (a screen touch, a UART round-trip, a motor move) ever blocking anything
else, especially the emergency button. This was solved by refactoring both boards from blocking,
sequential logic into explicit, non-blocking state machines ticked once per `loop()`. See
[state_machines.md](state_machines.md).

## 3. Hardware architecture

Two ESP32 microcontrollers connected by a hand-rolled ASCII UART protocol (115200 baud), one master
(touchscreen, motors, WiFi/Firebase, HTTP server) and one slave (sensors, emergency button,
extension motor). Full block diagram and pin-level detail:
[hardware_architecture.md](hardware_architecture.md).

## 4. Why two ESP32 boards

Pin budget. The master's soldered touchscreen alone uses 6 GPIOs; add UART and two motors and
there's exactly one spare pin left — not enough for a third motor. The extension motor was moved to
the second (otherwise mostly-idle) board and is driven remotely over UART. A genuine
resource-constraint-driven architectural decision, not an arbitrary split. See
[hardware_architecture.md](hardware_architecture.md).

## 5. Sensors

- **HC-SR04 ultrasonic** — gates braiding on the head/hair being within a calibrated distance range
  (7.0–16.0 cm).
- **TCS34725 RGB (I2C)** — resolves "match my hair" by comparing a raw RGB+Clear sample against 4
  calibrated reference colors (nearest-neighbor by squared Euclidean distance); deliberately uses
  raw values rather than Clear-normalized RGB, since normalizing would erase the exact signal that
  distinguishes black hair (low Clear) from blonde (high Clear).

## 6. Motor control

Three 28BYJ-48 steppers, driven two different ways: two via ULN2003 (unipolar — extension carousel,
braid motor) and one via an L298N (bipolar — rail motor), with intentionally different phase
tables that aren't interchangeable. All three are stepped non-blockingly (`STEP_DELAY_MS`-paced via
`millis()`, not `delay()`). See [mechanical_design.md](mechanical_design.md).

## 7. UART communication

A 13-message ASCII protocol over UART2, requester (master) using async request/poll with timeouts
rather than blocking waits. Carries distance/color sensor requests, the extension-motor command,
arm/disarm for the emergency button, and the emergency signal itself. See
[uart_protocol.md](uart_protocol.md).

## 8. Touchscreen

An ILI9341 TFT with resistive touch, driven by a third-party library (`TFT9341Touch`). Every screen
follows the same enter-once/poll-every-tick pattern as the rest of the state machine — including a
shared `touchEdge()` helper that fires once per new touch, replacing an earlier blocking
wait-for-release design.

## 9. React application

A Vite + React SPA (Login, Register, Dashboard, Booking, My Appointments, Admin pages) that talks
only to the master ESP32's HTTP API — never directly to Firebase. Has a small unit-test suite
(`api.test.js`) exercised in CI.

## 10. Firebase

Auth (email/password) + Cloud Firestore, but accessed **only** from the master ESP32's server side
— the app calls the ESP32, and the ESP32 calls Firebase's REST APIs. Collections: `users/{uid}`,
`codes/{code}` (temporary machine-entry codes, deleted on use to free the 4-digit space), and
`orders/{id}` (braid session history, status `completed` or `emergency`). If Firebase secrets
aren't configured, the master board runs in a documented demo mode (any code accepted, "Guest"
name, no network writes) — useful for hardware-only testing.

## 11. Mechanical system

Three subsystems: an extension carousel (dispenser), a rail (vertical positioning), and the braid
motor itself. See [mechanical_design.md](mechanical_design.md) for what's confirmed from the code,
including one explicitly-flagged known gap (the rail's segmented-lowering logic exists in
`Config.h` but isn't currently driven by anything during braiding).

## 12. 3D printing

Called out as part of the project's mechanical scope; no STL/STEP files are in the repository yet.
Placeholder folders exist at [hardware/mechanical/stl/](../hardware/mechanical/stl/) and
[hardware/mechanical/source/](../hardware/mechanical/source/) for when they're added.

## 13. Safety

A single physical button does double duty as confirm/all-clear (idle) and emergency stop
(mid-braid), gated by an `armed` flag set only during the braid step. On emergency: motors are
de-energized immediately, a distinct order status (`"emergency"`) is recorded, the temporary code is
kept valid for a retry, and the rail always returns home via the same recovery path as a normal
finish. Full detail, including what's *not* implemented (watchdog, timeouts, hardware cutoff):
[safety.md](safety.md).

## 14. Main engineering decisions worth discussing

- **Non-blocking refactor of both boards' firmware** — recognizing that blocking calls silently
  degrade safety-critical responsiveness (the emergency button), and fixing it with an explicit
  state-machine pattern applied consistently across every module (display, motors, UART, sensors).
- **Splitting a motor onto the "wrong" board** for a documented, verifiable reason (pin budget) —
  and building a small remote-motor-control protocol over UART to make it work, rather than forcing
  a specific board assignment.
- **Dual-core use on the master** — pinning the HTTP server to Core 0, separate from the session
  state machine on Core 1, so the web API never blocks on hardware state and vice versa.
- **Transparent handling of an unfinished feature** — the rail's segmented lowering is flagged
  in-code as not-yet-wired rather than silently shipped broken or hidden, which shaped how this
  documentation pass describes it too (see [state_machines.md](state_machines.md)).
