#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "sensors.h"

static unsigned long lastPub = 0;

#if ULTRASONIC_MODE_SIM
static float simVal = 80.0f; // valor de inicio en cm
#endif

void sensorsSetup() {
#if ULTRASONIC_MODE_SIM
  // semilla pseudoaleatoria basada en hardware
  uint32_t seed = esp_random();
  randomSeed(seed);
#else
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
#endif
}

float readUltrasonicCM() {
/*
#if ULTRASONIC_MODE_SIM
  // Caminata aleatoria acotada [MIN, MAX]
  int step = random(-5, 6); // -5..+5
  simVal += step;
  if (simVal < ULTRASONIC_MIN_CM) simVal = ULTRASONIC_MIN_CM;
  if (simVal > ULTRASONIC_MAX_CM) simVal = ULTRASONIC_MAX_CM;
  return simVal;
#else
*/
  // Pulso de 10us en TRIG
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  // Medimos HIGH en ECHO con timeout (~30 ms ≈ ~5 m)
  unsigned long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return NAN; // timeout, sin eco
  float cm = duration * 0.0343f / 2.0f; // velocidad sonido ~343 m/s
  if (cm < ULTRASONIC_MIN_CM || cm > ULTRASONIC_MAX_CM) return NAN;
  return cm;
#endif
}

void publishUltrasonicIfDue(PubSubClient& mqtt) {
  unsigned long now = millis();
  if (now - lastPub < ULTRASONIC_PUBLISH_MS) return;
  lastPub = now;

  float d = readUltrasonicCM();

  StaticJsonDocument<160> doc;
  doc["ts_ms"] = now;
  if (isnan(d)) {
    doc["distance_cm"] = nullptr;
  } else {
    doc["distance_cm"] = d;
  }

  String payload;
  serializeJson(doc, payload);

  if (mqtt.connected()) {
    mqtt.publish(MQTT_TOPIC_DIST, payload.c_str(), true);
  }
}
