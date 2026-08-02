// ============================================================================
//  Config.h  —  ESP32 "שני" (Master). כל הפינים והקבועים במקום אחד.
// ============================================================================
#ifndef CONFIG_H
#define CONFIG_H

#include <Firebase_ESP_Client.h>
#include "Secrets.h"   // WiFi + Firebase — קובץ נפרד שלא נכנס לגיט, ראה Secrets.h.example

// ---------- מסך TFT (SPI קבוע: SCK=18, MOSI=23, MISO=19) ----------
#define TFT_CS    5                  // Chip Select של המסך
#define TFT_DC    4                  // Data/Command
#define TFT_TCS   15                 // Chip Select של המגע
#define TFT_TIRQ  35                 // קו פסיקת המגע (input-only)

// ---------- מנוע מסילה (ULN2003 / 28BYJ-48) ----------
//   נקי לגמרי — אין שום התנגשות.
#define RAIL_IN1  26                 // סליל 1
#define RAIL_IN2  25                 // סליל 2
#define RAIL_IN3  14                 // סליל 3
#define RAIL_IN4  13                 // סליל 4

// ---------- מנוע קליעה (מנוע צעד 1) ----------
#define BRAID_IN1 27
#define BRAID_IN2 33
#define BRAID_IN3 32
#define BRAID_IN4 3                  // GPIO3=RX0, 

// ---------- UART אל הלוח הראשון (Serial2 על פינים רגילים 16/17) ----------
#define UART_TX_PIN 17               // GPIO17 = TX
#define UART_RX_PIN 16               // GPIO16 = RX
#define UART_BAUD   115200

// ---------- פרמטרי מנועים ----------
//   הועלה מ-3 ל-8: קצב מהיר מדי גורם לרוטור "לרעוד" במקום להתקדם (מומנט
//   אפקטיבי נמוך). אם עדיין אין מספיק כוח — אפשר להעלות עוד (10-15).
#define STEP_DELAY_MS      3

// ---------- מסילה: ירידה במקטעים (segments) ----------
#define RAIL_SEGMENT_STEPS 400       // צעדים במקטע ירידה בודד)
#define RAIL_MAX_SEGMENTS  7         // מספר מקטעי הירידה המקסימלי (סוף המסילה)

// ---------- מיפוי תוספות (מקומות בקרוסלה, סדר זהה לצבעים בלוח הראשון) ----------
#define EXT_BLONDE  0
#define EXT_GREEN   1
#define EXT_RED     2
#define EXT_BLACK   3
#define EXT_NONE   -1                // בלי תוספות
#define EXT_MYHAIR  99              // "כצבע שערי" — ייפתר בעזרת חיישן הצבע
#define MAX_EXTENSIONS 3

// ---------- WiFi + Firebase ----------
// הערכים עצמם ב-Secrets.h (לא בגיט) — ראה include למעלה.
#define CODE_LENGTH        4         // אורך הקוד הזמני

#endif
