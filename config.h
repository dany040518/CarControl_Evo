#pragma once

// ================= Motores, Luces y Bocina =================
#define LUCES      22
#define BOCINA     21

#define ENA_IZQ    15
#define IN1_IZQ     4
#define IN2_IZQ    16

#define ENA_DER    19
#define IN1_DER    17
#define IN2_DER    18

// ================= PWM =================
#define PWM_FREQ_HZ    20000
#define PWM_RES_BITS   10
#define PWM_CANAL_IZQ   0
#define PWM_CANAL_DER   1

// ================= WiFi / AP =================
#define AP_SSID   "ESP32_Config"
#define AP_PASS   ""

// ================= MQTT =================
#define MQTT_BROKER        "test.mosquitto.org"
#define MQTT_PORT          1883
#define MQTT_CLIENT_ID     "ESP32CarClient"
#define MQTT_TOPIC_CMD     "carro/instrucciones"
#define MQTT_TOPIC_DIST    "carro/telemetria/distancia"

// ================= Publicación Ultrasonido =================
#define ULTRASONIC_MODE_SIM     1
#define ULTRASONIC_PUBLISH_MS   1000
#define ULTRASONIC_MIN_CM       2
#define ULTRASONIC_MAX_CM       400

// -------------------- HC-SR04 Pins --------------
#ifndef ULTRASONIC_TRIG_PIN
  #define ULTRASONIC_TRIG_PIN 35  // Trigger (OUTPUT)
#endif
#ifndef ULTRASONIC_ECHO_PIN
  #define ULTRASONIC_ECHO_PIN 34  // Echo (INPUT) -> 3.3V (usar divisor si aplica)
#endif

// (opcional) alias const
static const int trigPin = ULTRASONIC_TRIG_PIN;
static const int echoPin = ULTRASONIC_ECHO_PIN;
