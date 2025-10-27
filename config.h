#pragma once

// ================= Pines Motores y Aux =================
#define LUCES_PIN      22
#define BOCINA_PIN     21

#define ENA_IZQ_PIN    15
#define IN1_IZQ_PIN     4
#define IN2_IZQ_PIN    16

#define ENA_DER_PIN    19
#define IN1_DER_PIN    17
#define IN2_DER_PIN    18

// ================= Ultrasonido HC-SR04 =================
#define ULTRASONIC_TRIG_PIN  5
#define ULTRASONIC_ECHO_PIN  2

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
#define MQTT_TOPIC_CMD     "carro/instrucciones"        // comandos de movimiento (ya lo usas)
#define MQTT_TOPIC_DIST    "carro/telemetria/distancia" // NUEVO: telemetría de distancia

// ================= Publicación Ultrasonido =================
#define ULTRASONIC_MODE_SIM     1       // 1 = simula sin hardware, 0 = usa HC-SR04 real
#define ULTRASONIC_PUBLISH_MS   1000    // periodo de publicación en ms
#define ULTRASONIC_MIN_CM       2
#define ULTRASONIC_MAX_CM       400