// ============================================================================
//  TCS34725_Color_sensor.ino  —  חיישן צבע TCS34725 (I2C) — ESP32 ראשון
// ============================================================================
#include "Config.h"              
#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <limits.h>

static Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
static bool sensorOk = false;

// RAW R,G,B,Clear.
struct ColorRef { const char* name; int r, g, b, c; };
static ColorRef colorRefs[] = {
  {"Blonde", 65, 77, 60, 200},
  {"Green",  51, 73, 49, 171},
  {"Red",    92, 74, 63, 225},
  {"Black",  51, 61, 50, 160}
};
static const int numColors = 4;
static const long MAX_COLOR_DIST = 6000;   

// אתחול חיישן הצבע
void colorSensorSetup() {
  pinMode(COLOR_LED_PIN, OUTPUT);
  digitalWrite(COLOR_LED_PIN, LOW);       // כבוי כברירת מחדל
  Wire.begin(COLOR_SDA, COLOR_SCL);
  sensorOk = tcs.begin();
  if (!sensorOk) Serial.println("[FIRST] TCS34725 NOT FOUND — check wiring/I2C address");
  else           Serial.println("[FIRST] TCS34725 ready");
}

static const char* findClosestColor(int r, int g, int b, int c) {
  long minDist = LONG_MAX;
  const char* best = "Unknown";
  for (int i = 0; i < numColors; i++) {
    long dr = r - colorRefs[i].r, dg = g - colorRefs[i].g,
         db = b - colorRefs[i].b, dc = c - colorRefs[i].c;
    long dist = dr*dr + dg*dg + db*db + dc*dc;   // מרחק אוקלידי ב-4 מימדים
    if (dist < minDist) { minDist = dist; best = colorRefs[i].name; }
  }
  if (minDist > MAX_COLOR_DIST) {
    Serial.printf("[FIRST] Color too far from any reference (dist=%ld > %ld) -> Unknown\n",
                  minDist, MAX_COLOR_DIST);
    return "Unknown";
  }
  return best;
}

// שם הצבע הקרוב
String readHairColor() {
  if (!sensorOk) { Serial.println("[FIRST] Color sensor not initialized"); return "Unknown"; }

  Serial.println("[FIRST] Action: scanning hair color (single sample)...");
  digitalWrite(COLOR_LED_PIN, HIGH);      // מדליקים רק לזמן המדידה
  delay(20);                              // רגע להתייצבות התאורה
  uint16_t rRaw, gRaw, bRaw, clear;
  tcs.getRawData(&rRaw, &gRaw, &bRaw, &clear);
  digitalWrite(COLOR_LED_PIN, LOW);       // מכבים מייד אחרי המדידה
  int r = rRaw, g = gRaw, b = bRaw, c = clear;

  const char* name = findClosestColor(r, g, b, c);
  Serial.printf("[FIRST] Result: RAW=(R=%d,G=%d,B=%d,C=%d) -> detected color = %s\n", r, g, b, c, name);
  return String(name);
}
