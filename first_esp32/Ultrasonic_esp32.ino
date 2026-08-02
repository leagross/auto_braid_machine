// ============================================================================
// בודק אם שיער הראש במרחק תקין
// ============================================================================
#include "Config.h"               

// אתחול החיישן
void ultrasonicSetup() {
  pinMode(TRIG_PIN, OUTPUT);        
  pinMode(ECHO_PIN, INPUT);        
  digitalWrite(TRIG_PIN, LOW);      
}

// מדידה בודדת בס"מ. מחזיר -1 אם אין הד
float readDistanceCm() {
  Serial.println("[FIRST] Action: measuring distance (ultrasonic)...");

  digitalWrite(TRIG_PIN, LOW);      
  delayMicroseconds(2);             // ייצוב קצר
  digitalWrite(TRIG_PIN, HIGH);     // מתחיל פולס טריגר
  delayMicroseconds(10);            
  digitalWrite(TRIG_PIN, LOW);      

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // משך ההד (timeout 30ms)
  if (duration == 0) {              
    Serial.println("[FIRST] Result: NO ECHO (out of range)");
    return -1.0;
  }

  // מהירות הקול ~0.0343 ס"מ
  float dist = duration * 0.0343 / 2.0;
  bool ok = (dist >= DIST_MIN_CM && dist <= DIST_MAX_CM);
  Serial.printf("[FIRST] Result: duration=%ldus, distance=%.1fcm, OK=%s\n",
                duration, dist, ok ? "YES" : "NO");
  return dist;
}
