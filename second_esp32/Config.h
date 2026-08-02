// ============================================================================
//  Config.h — "second" ESP32 (Master). All pins and constants in one place.
// ============================================================================
#ifndef CONFIG_H
#define CONFIG_H

#include <Firebase_ESP_Client.h>
#include "Secrets.h"   // WiFi + Firebase — separate file, not in git, see Secrets.h.example

// ---------- TFT screen (fixed SPI pins: SCK=18, MOSI=23, MISO=19) ----------
#define TFT_CS    5                  // screen chip select
#define TFT_DC    4                  // data/command
#define TFT_TCS   15                 // touch controller chip select
#define TFT_TIRQ  35                 // touch interrupt line (input-only)

// ---------- Rail motor (ULN2003 / 28BYJ-48) ----------
//   Fully clean — no conflicts.
#define RAIL_IN1  26                 // coil 1
#define RAIL_IN2  25                 // coil 2
#define RAIL_IN3  14                 // coil 3
#define RAIL_IN4  13                 // coil 4

// ---------- Braid motor (stepper 1) ----------
#define BRAID_IN1 27
#define BRAID_IN2 33
#define BRAID_IN3 32
#define BRAID_IN4 3                  // GPIO3=RX0

// ---------- UART to the first board (Serial2 on regular pins 16/17) ----------
#define UART_TX_PIN 17               // GPIO17 = TX
#define UART_RX_PIN 16               // GPIO16 = RX
#define UART_BAUD   115200

// ---------- Motor parameters ----------
//   Too fast a rate makes the rotor "shudder" in place instead of turning
//   (low effective torque). If there's still not enough force, try raising
//   it further (10-15).
#define STEP_DELAY_MS      3

// ---------- Rail: lowering in segments ----------
#define RAIL_SEGMENT_STEPS 400       // steps per lowering segment
#define RAIL_MAX_SEGMENTS  7         // max number of lowering segments (bottom of the rail)

// ---------- Extension mapping (carousel positions, same order as the colors on the first board) ----------
#define EXT_BLONDE  0
#define EXT_GREEN   1
#define EXT_RED     2
#define EXT_BLACK   3
#define EXT_NONE   -1                // no extension
#define EXT_MYHAIR  99               // "match my hair" — resolved via the color sensor
#define MAX_EXTENSIONS 3

// ---------- WiFi + Firebase ----------
// Actual values live in Secrets.h (not in git) — see include above.
#define CODE_LENGTH        4         // temporary code length

#endif
