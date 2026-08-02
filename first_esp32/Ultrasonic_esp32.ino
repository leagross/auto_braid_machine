// ============================================================================
//  Ultrasonic_esp32.ino — checks whether the head/hair is at a valid distance
// ============================================================================
#include "Config.h"

// Sensor init
void ultrasonicSetup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
}

// Single measurement in cm. Returns -1 if no echo was received.
float readDistanceCm() {
  Serial.println("[FIRST] Action: measuring distance (ultrasonic)...");

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);             // brief settle
  digitalWrite(TRIG_PIN, HIGH);     // start the trigger pulse
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // echo duration (30ms timeout)
  if (duration == 0) {
    Serial.println("[FIRST] Result: NO ECHO (out of range)");
    return -1.0;
  }

  // speed of sound ~0.0343 cm/us
  float dist = duration * 0.0343 / 2.0;
  bool ok = (dist >= DIST_MIN_CM && dist <= DIST_MAX_CM);
  Serial.printf("[FIRST] Result: duration=%ldus, distance=%.1fcm, OK=%s\n",
                duration, dist, ok ? "YES" : "NO");
  return dist;
}
