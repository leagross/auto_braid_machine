# Bill of Materials

Physical components used to build AutoBraid.

## Development boards

| Component | Quantity | Purpose |
|---|---|---|
| CS380M development board | 1 | Candidate board for one of the two ESP32 controllers |
| CS-ESP32-T development board (2.8" touch display) | 1 | Master controller — drives the touchscreen UI |
| ESP32 DOIT DEVKIT V1 (USB Type-C) | 1 | Candidate board for the sensor/slave controller |

## Sensors & input

| Component | Quantity | Purpose |
|---|---|---|
| HC-SR04 ultrasonic distance sensor | 1 | Checks hair/head positioning before braiding starts |
| TCS34725 RGB color sensor module | 1 | Detects hair color for the "match my hair" extension option |
| Momentary push button | 1 | Start confirmation and emergency stop |
| ILI9341 touch display, 2.8"–3.2" | 1 | Touchscreen user interface |

## Motors & drivers

| Component | Quantity | Purpose |
|---|---|---|
| 28BYJ-48 5V 4-phase DC gear stepper motor | 3 | Extension carousel, braid mechanism, rail mechanism |
| ULN2003 driver | 2 | Drives the extension motor and the braid motor |
| L298N motor driver | 1 | Drives the rail motor |

## Power & wiring

| Component | Purpose |
|---|---|
| 5V supply for motors/drivers | Powers the stepper motors and driver boards |
| Shared ground across boards and drivers | Required for reliable communication and motor control |

## Not physical hardware, but part of the system

| Component | Role |
|---|---|
| Firebase (Auth + Cloud Firestore) | Backend for login, machine access codes, and order history |
| React web app | Customer/staff interface for registration, login, and order history |

---

Not all quantities and specifications (exact board model per role, power supply ratings, enclosure
hardware) are finalized yet — this list will be updated as the physical build is finalized.
