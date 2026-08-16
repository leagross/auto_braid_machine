# Bill of Materials

Physical parts used to build AutoBraid. Pin assignments are in
[README.md](../README.md#-wiring-pin-map); this is the parts list behind them. For a narrative
version of the same information (with driver/phase-table notes), see [components.md](../components.md)
at the repository root — that file predates this one and is kept for existing links.

Values are taken from the code (`Config.h` in both sketches, `Motors.ino`) and from
`components.md`. Where the exact model/quantity isn't confirmed anywhere in the repo, it's marked
`Not documented yet` rather than guessed.

## Development boards

| Component | Model | Quantity | Purpose | Controlled By | Interface |
|---|---|---|---|---|---|
| Microcontroller ("first" board) | ESP32 dev board (WROOM-32 or similar) — CS380M development board or ESP32 DOIT DEVKIT V1 (USB Type-C) | 1 | Runs `first_esp32`: button, ultrasonic sensor, color sensor, extension/dispenser motor | — | USB (flashing/Serial), GPIO, I2C, UART2 |
| Microcontroller ("second" board) | CS-ESP32-T development board with 2.8" touch display | 1 | Runs `second_esp32`: touchscreen UI, rail motor, braid motor, WiFi, Firebase, HTTP server | — | USB (flashing/Serial), SPI, GPIO, UART2, WiFi |

> The task brief lists three candidate board models (CS380M, CS-ESP32-T with 2.8" touch display,
> ESP32 DOIT DEVKIT V1 USB-C). The repository code does not pin down which physical board is used
> for which role beyond "the board with the soldered touchscreen is the second/master board" — see
> the wiring notes in [README.md](../README.md#-wiring-pin-map). Confirm exact board models against
> the owner's actual hardware.

## Sensors & input

| Component | Model | Quantity | Purpose | Controlled By | Interface |
|---|---|---|---|---|---|
| Ultrasonic distance sensor | HC-SR04 | 1 | Checks the head/hair is within `DIST_MIN_CM`–`DIST_MAX_CM` (7.0–16.0 cm, `first_esp32/Config.h`) before braiding starts | first board (`Ultrasonic_esp32.ino`) | TRIG=GPIO21 (out), ECHO=GPIO19 (in) |
| RGB color sensor | TCS34725 (I2C breakout, e.g. Adafruit) | 1 | Resolves the "match my hair" extension color from raw RGB+Clear samples | first board (`TCS34725_Color_sensor.ino`) | I2C — SDA=GPIO32, SCL=GPIO26; LED enable=GPIO33 |
| Start / emergency button | Momentary push button, normally-open | 1 | Dual role: confirms actions when idle, triggers emergency stop mid-braid | first board (`buttom_switch.ino`) | GPIO27 (`INPUT_PULLUP`) |
| Touch display | ILI9341 TFT, 2.8"–3.2", SPI, resistive touch (via `TFT9341Touch` library, Avi Hayun) | 1 | Full UI: keypad, extension selection, status/emergency/done screens | second board (`DisplayManager.ino`) | SPI (VSPI) — SCK=18, MOSI=23, MISO=19, CS=5, DC=4, touch CS=15, touch IRQ=35 |

## Motors & drivers

| Component | Model | Quantity | Purpose | Controlled By | Interface |
|---|---|---|---|---|---|
| Stepper motor | 28BYJ-48, 5V, 4-phase unipolar | 3 | One per mechanism: extension carousel (dispenser), braid motor, rail motor | first board (dispenser) + second board (braid, rail) | 4 digital outputs each, via a driver |
| Unipolar stepper driver | ULN2003 driver board | 2 | Drives the dispenser motor (first board) and the braid motor (second board) | first board, second board | 4 GPIOs each: dispenser=`2,15,18,5`; braid=`27,33,32,3` |
| Dual H-bridge driver | L298N | 1 | Drives the rail motor **in bipolar mode** — a different phase table from the ULN2003 motors (see `RAIL_PHASE_SEQ` vs. `PHASE_SEQ` in `second_esp32/Motors.ino`); not interchangeable | second board | 4 GPIOs: `26,25,14,13` |

## Power & wiring

| Component | Model | Quantity | Purpose | Controlled By | Interface |
|---|---|---|---|---|---|
| 5V supply for motors/drivers | Not documented yet | Not documented yet | Powers the 28BYJ-48 + ULN2003 boards (standard 5V logic/coil supply) | — | — |
| Power supply for the L298N | Not documented yet — check the specific board's input range (commonly 6–12V with an onboard 5V regulator) against its datasheet before powering it | 1 | Powers the rail motor driver | — | — |
| Shared ground | Jumper wire(s) | — | Mandatory shared GND between both ESP32 boards and all motor drivers — required for both UART framing and motor driver logic to work correctly | — | — |
| Enclosure / mounting hardware | Not documented yet | Not documented yet | Mechanical build — not pinned down anywhere in the code | — | — |

## Not physical hardware, but part of the system

| Component | Role |
|---|---|
| Firebase project (Auth + Cloud Firestore) | Backs the app's login, temporary codes, and order history — see `second_esp32/Secrets.h.example` |
| React web app (`react-app/`) | Staff/customer interface — registration, login, generating a machine code, viewing order history — talks only to the second board's HTTP server over the local network |

## Notes

- Quantities and pin numbers above are taken directly from `first_esp32/Config.h`,
  `second_esp32/Config.h`, and `second_esp32/Motors.ino` — they are not estimates.
- Owner TODO: confirm exact board part numbers/links and the motor/L298N power supply
  specifications, and replace the `Not documented yet` cells above. See
  [DOCUMENTATION_UPGRADE_REPORT.md](../docs/DOCUMENTATION_UPGRADE_REPORT.md) for the full TODO list.
