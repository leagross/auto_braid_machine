# Components (Bill of Materials)

Everything used to build the physical machine. Pin assignments are in [README.md](README.md#-wiring-pin-map);
this is the parts list behind them.

## Compute

| Qty | Component | Role |
|---|---|---|
| 2 | ESP32 dev board (WROOM-32 or similar, USB-programmable) | one runs `first_esp32`, the other `second_esp32` — see [README.md](README.md) for which sketch goes where |

## Sensors & input

| Qty | Component | Used by | Notes |
|---|---|---|---|
| 1 | HC-SR04 ultrasonic distance sensor | first board | checks the head/hair is within `DIST_MIN_CM`–`DIST_MAX_CM` before braiding starts |
| 1 | TCS34725 RGB color sensor (I2C breakout, e.g. Adafruit) | first board | resolves the "match my hair" extension color |
| 1 | Momentary push button (normally-open) | first board | start confirm / emergency stop (dual role, see [function_map.md](function_map.md)) |
| 1 | ILI9341 TFT display, 2.8"–3.2", SPI, with resistive touch | second board | requires the `TFT9341Touch` library (by Avi Hayun) — not in the public Library Manager, see README |

## Motors & drivers

| Qty | Component | Drives | Notes |
|---|---|---|---|
| 3 | 28BYJ-48 stepper motor (5V unipolar) | extension carousel, braid motor, rail motor | the rail motor's 28BYJ-48 is wired/driven in bipolar mode through the L298N below — see `second_esp32/Motors.ino` for the driver-specific phase tables, they are **not** interchangeable |
| 2 | ULN2003 driver board | extension motor (first board), braid motor (second board) | standard unipolar stepper driver, sold paired with the 28BYJ-48 |
| 1 | L298N dual H-bridge driver board | rail motor (second board) | bipolar drive — different phase table from the ULN2003 motors, see the warning comment in `Motors.ino` |

## Power & wiring

| Component | Notes |
|---|---|
| 5V supply for the motors/drivers | 28BYJ-48 + ULN2003 boards are standard 5V; check your specific L298N board's input range (commonly 6–12V, with an onboard 5V regulator) — verify against your board's datasheet before powering it |
| Shared GND across both boards + all drivers | mandatory — see the UART wiring warning in README |
| Jumper wires / perfboard / enclosure | mechanical build, not pinned down by the code |

## Not physical hardware, but part of the system

| Component | Role |
|---|---|
| Firebase project (Auth + Cloud Firestore) | backs the app's login, temporary codes, and order history — see `second_esp32/Secrets.h.example` |
| A machine running the React app (`react-app/`) | staff/customer interface, talks to the second board's HTTP server over the local network |
