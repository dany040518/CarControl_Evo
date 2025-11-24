#pragma once

// ================= Pines Motores y Aux =================
#define LUCES      22

#define ENA_IZQ    33
#define IN1_IZQ    25
#define IN2_IZQ    26

#define ENA_DER    12
#define IN1_DER    27
#define IN2_DER    14

// ================= PWM Motores =================
#define PWM_FREQ_HZ    1000
#define PWM_RES_BITS   10
#define PWM_CANAL_IZQ   0
#define PWM_CANAL_DER   1

// ================= WiFi / AP =================
#define AP_SSID   "ESP32_Config"
#define AP_PASS   ""

// ================= MQTT =================
//#define MQTT_BROKER        "test.mosquitto.org"
#define AWS_IOT_ENDPOINT "a2hfirfjvweeu1-ats.iot.us-east-1.amazonaws.com"
#define MQTT_PORT        8883 
#define MQTT_CLIENT_ID     "ESP32CarClient"
#define MQTT_TOPIC_CMD     "carro/instrucciones"
#define MQTT_TOPIC_DIST    "carro/telemetria/distancia"

// ================= Servo base ultrasonido =================
#define SERVO_PIN          13    
#define SERVO_MIN_ANGLE    45    
#define SERVO_MAX_ANGLE    135   
#define SERVO_STEP_DEG     5     
#define SERVO_STEP_MS      150   

// PWM específico del SERVO
#define SERVO_FREQ         50     // 50 Hz (servos estándar)
#define SERVO_BITS         16     // resolución de PWM para el servo

// ================= Publicación Ultrasonido =================
#define ULTRASONIC_MODE_SIM     0
#define ULTRASONIC_PUBLISH_MS   250
#define ULTRASONIC_MIN_CM       2
#define ULTRASONIC_MAX_CM       400
#define ULTRASONIC_SAMPLES      3

// -------------------- HC-SR04 Pins --------------
#ifndef ULTRASONIC_TRIG_PIN
  #define ULTRASONIC_TRIG_PIN 5  // Trigger (OUTPUT)
#endif
#ifndef ULTRASONIC_ECHO_PIN
  #define ULTRASONIC_ECHO_PIN 18  // Echo (INPUT)
#endif

// (opcional) alias const
static const int trigPin = ULTRASONIC_TRIG_PIN;