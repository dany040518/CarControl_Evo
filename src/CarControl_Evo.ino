#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "sensors.h"
#include <WiFiClientSecure.h>
#include "certs_aws.h"
//#include "certs.h"

// ====================== CREDENCIALES STA ======================
const char* STA_SSID = "Danna’s iPhone";
const char* STA_PASS = "05182004...";

const int VELOCIDAD_PWM = 400;


// ====================== ESTADO DEL CARRO ======================
String estadoCarro = "STOP";

// ====================== SERVO BASE ULTRASONIDO ======================
int servoAngle = 90;
int servoDir   = 1;
unsigned long ultimoMovimientoServo = 0;

// Convierte ángulo (0-180) a duty (0 - 2^SERVO_BITS-1)
uint16_t angleToDuty(int ang) {
  if (ang < 0)   ang = 0;
  if (ang > 180) ang = 180;

  const float minUs = 500.0;
  const float maxUs = 2400.0;
  const float periodUs = 1000000.0 / SERVO_FREQ;   // 20ms -> 20000us

  float pulseUs = minUs + (maxUs - minUs) * ((float)ang / 180.0);
  float duty = (pulseUs / periodUs) * ((1 << SERVO_BITS) - 1);

  return (uint16_t)duty;
}

// ====================== WIFI & MQTT ===========================
WiFiClientSecure espClientSecure;
PubSubClient mqttClient(espClientSecure);

// ====================== PROTOTIPOS ============================
void motoresStop();
void motoresAdelante();
void motoresAtras();
void motoresIzquierda();
void motoresDerecha();

// ====================== SETUP MOTORES (LEDc estilo tuyo) ======
void setupMotores() {
  pinMode(LUCES, OUTPUT);

  pinMode(IN1_IZQ, OUTPUT);
  pinMode(IN2_IZQ, OUTPUT);
  pinMode(IN1_DER, OUTPUT);
  pinMode(IN2_DER, OUTPUT);

  ledcAttach(ENA_IZQ, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttach(ENA_DER, PWM_FREQ_HZ, PWM_RES_BITS);

  ledcWrite(ENA_IZQ, 0);
  ledcWrite(ENA_DER, 0);

  Serial.println("[init] Pines motores configurados");
}

// ====================== FUNCIONES MOTORES =====================
void motoresStop() {
  estadoCarro = "STOP";

  digitalWrite(IN1_IZQ, LOW);
  digitalWrite(IN2_IZQ, LOW);
  digitalWrite(IN1_DER, LOW);
  digitalWrite(IN2_DER, LOW);

  ledcWrite(ENA_IZQ, 0);
  ledcWrite(ENA_DER, 0);
}

void motoresAdelante() {
  estadoCarro = "ADELANTE";

  digitalWrite(IN1_IZQ, HIGH);
  digitalWrite(IN2_IZQ, LOW);

  digitalWrite(IN1_DER, HIGH);
  digitalWrite(IN2_DER, LOW);

  ledcWrite(ENA_IZQ, VELOCIDAD_PWM);
  ledcWrite(ENA_DER, VELOCIDAD_PWM);
}

void motoresAtras() {
  estadoCarro = "ATRAS";

  digitalWrite(IN1_IZQ, LOW);
  digitalWrite(IN2_IZQ, HIGH);

  digitalWrite(IN1_DER, LOW);
  digitalWrite(IN2_DER, HIGH);

  ledcWrite(ENA_IZQ, VELOCIDAD_PWM);
  ledcWrite(ENA_DER, VELOCIDAD_PWM);
}

void motoresIzquierda() {
  estadoCarro = "IZQUIERDA";

  digitalWrite(IN1_IZQ, LOW);
  digitalWrite(IN2_IZQ, LOW);
  ledcWrite(ENA_IZQ, 0);

  digitalWrite(IN1_DER, HIGH);
  digitalWrite(IN2_DER, LOW);
  ledcWrite(ENA_DER, VELOCIDAD_PWM);
}

void motoresDerecha() {
  estadoCarro = "DERECHA";

  digitalWrite(IN1_IZQ, HIGH);
  digitalWrite(IN2_IZQ, LOW);
  ledcWrite(ENA_IZQ, VELOCIDAD_PWM);

  digitalWrite(IN1_DER, LOW);
  digitalWrite(IN2_DER, LOW);
  ledcWrite(ENA_DER, 0);
  
}

// ====================== ULTRASONIDO ===========================
long leerDistanciaRealCM() {

  long suma = 0;
  int validos = 0;

  for (int i = 0; i < ULTRASONIC_SAMPLES; i++) {

    // Disparo del TRIG
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    delayMicroseconds(3);
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

    long dur = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 25000);  
    long dist = dur * 0.0343 / 2;

    if (dist > ULTRASONIC_MIN_CM && dist < ULTRASONIC_MAX_CM) {
      suma += dist;
      validos++;
    }

    delay(5); // pequeña pausa entre muestras
  }

  if (validos == 0) return -1;

  return suma / validos;  // promedio
}


long leerDistanciaCM() {
#if ULTRASONIC_MODE_SIM
  // Modo simulado por ahora (lo puedes apagar poniendo ULTRASONIC_MODE_SIM 0)
  static int fake = 50;
  fake += (random(-5, 6));
  if (fake < 10) fake = 10;
  if (fake > 200) fake = 200;
  return fake;
#else
  return leerDistanciaRealCM();
#endif
}

// ====================== WIFI (AP + STA) =======================
void iniciarAP() {
  WiFi.mode(WIFI_AP_STA);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP: ");
  Serial.print(AP_SSID);
  Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FALLO");
  Serial.print("IP AP: ");
  Serial.println(WiFi.softAPIP());
}

void iniciarWiFiSta() {
  Serial.println("Conectando en modo STA…");
  WiFi.begin(STA_SSID, STA_PASS);
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi STA conectado!");
    Serial.print("IP STA: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No se pudo conectar como STA (seguirá solo AP).");
  }
}

// ====================== MQTT ==============================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String msg = String((char*)payload);

  Serial.print("MQTT [");
  Serial.print(topic);
  Serial.print("] -> ");
  Serial.println(msg);

  msg.toUpperCase();

  if (msg == "ADELANTE")       motoresAdelante();
  else if (msg == "ATRAS")     motoresAtras();
  else if (msg == "IZQUIERDA") motoresIzquierda();
  else if (msg == "DERECHA")   motoresDerecha();
  else if (msg == "STOP")      motoresStop();
  else if (msg == "LUCES_ON")  digitalWrite(LUCES, HIGH);
  else if (msg == "LUCES_OFF") digitalWrite(LUCES, LOW);
}

void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando a MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("OK");
      mqttClient.subscribe(MQTT_TOPIC_CMD);
      Serial.print("Subscrito a: ");
      Serial.println(MQTT_TOPIC_CMD);
    } else {
      Serial.print("FALLO, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" reintentando en 2s");
      delay(2000);
    }
  }
}

// ====================== SETUP ==============================
unsigned long ultimoEnvio = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  // Motores y luces (tu esquema con ledcAttach)
  setupMotores();

// ===== Servo base ultrasonido (PWM con ledcAttach) =====
  ledcAttach(SERVO_PIN, SERVO_FREQ, SERVO_BITS);

  servoAngle = (SERVO_MIN_ANGLE + SERVO_MAX_ANGLE) / 2;   // centro del barrido
  ledcWrite(SERVO_PIN, angleToDuty(servoAngle));
  delay(500);

  // TEST rápido: izquierda, derecha, centro
  ledcWrite(SERVO_PIN, angleToDuty(SERVO_MIN_ANGLE));
  delay(500);
  ledcWrite(SERVO_PIN, angleToDuty(SERVO_MAX_ANGLE));
  delay(500);
  ledcWrite(SERVO_PIN, angleToDuty(servoAngle));
  delay(500);


  // Ultrasonido
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);

  // Apagar luces al inicio
  digitalWrite(LUCES, LOW);
  motoresStop();

  // WiFi
  iniciarAP();
  iniciarWiFiSta();

  // TLS sin validación de certificados
  //espClientSecure.setInsecure();
  //TLS con certificados de mosquitto  
  //espClientSecure.setCACert(ROOT_CA);
  // MQTT mosquitto
  //mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  //TLS para AWS IoT
  espClientSecure.setCACert(AWS_CERT_CA);
  espClientSecure.setCertificate(AWS_CERT_CRT);
  espClientSecure.setPrivateKey(AWS_CERT_PRIVATE);

  // Endpoint y puerto de AWS IoT
  mqttClient.setServer(AWS_IOT_ENDPOINT, MQTT_PORT);

}

// ====================== LOOP ==============================
void loop() {
  if (!mqttClient.connected()) {
    conectarMQTT();
  }
  mqttClient.loop();

  unsigned long ahora = millis();

  // --------- MOVER SERVO EN BARRIDO ---------
  if (ahora - ultimoMovimientoServo >= SERVO_STEP_MS) {
    ultimoMovimientoServo = ahora;

    servoAngle += servoDir * SERVO_STEP_DEG;

    if (servoAngle >= SERVO_MAX_ANGLE) {
      servoAngle = SERVO_MAX_ANGLE;
      servoDir = -1;
    } else if (servoAngle <= SERVO_MIN_ANGLE) {
      servoAngle = SERVO_MIN_ANGLE;
      servoDir = 1;
    }

    ledcWrite(SERVO_PIN, angleToDuty(servoAngle));
  }


  if (ahora - ultimoEnvio >= ULTRASONIC_PUBLISH_MS) {
    ultimoEnvio = ahora;

    long distancia = leerDistanciaCM();

    String payload = "{";
    payload += "\"distancia\":" + String(distancia) + ",";
    payload += "\"estado\":\"" + estadoCarro + "\",";
    payload += "\"luces\":" + String(digitalRead(LUCES)) + ",";
    payload += "\"ts\":" + String(millis());
    payload += "}";


    mqttClient.publish(MQTT_TOPIC_DIST, payload.c_str());
    Serial.print("Telemetria -> ");
    Serial.println(payload);
  }
}