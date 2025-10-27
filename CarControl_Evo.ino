#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "sensors.h"

// ---------------- Luces y Bocina ----------------
bool lucesEncendidas = false;
bool sonando = false;
bool sonidoRev = false;
unsigned long ultimaAlarma = 0;
const unsigned long intervaloBeep = 500;

/*
const int LUCES = 22;
const int BOCINA = 21;

const int ENA_IZQ = 15;
const int IN1_IZQ = 4;
const int IN2_IZQ = 16;

const int ENA_DER = 19;
const int IN1_DER = 17;
const int IN2_DER = 18;

// PWM
const int PWM_FREQ_HZ = 20000;
const int PWM_RES_BITS = 10;
const int PWM_CANAL_IZQ = 0;
const int PWM_CANAL_DER = 1;

// ---- WiFi / HTTP / DNS / MQTT ----
const char* apSSID = "ESP32_Config";
const char* apPass = "";
*/

WiFiClient espClient;
WebServer server(80);

const byte DNS_PORT = 53;
DNSServer dnsServer;

PubSubClient mqttClient(espClient);
/*
const char* mqtt_server = "test.mosquitto.org";
const char* mqtt_topic = "carro/instrucciones";
*/

Preferences preferences;

String savedSSID = "";
String savedPass = "";

bool shouldRestartAfterConnect = false;
unsigned long lastScanTime = 0;

// forward declarations (handler functions implemented más abajo)
String indexPage();
void handleRoot();
void handleScan();
void handleConnect();
void handleStatus();
void handleForget();
void handleMove();

// ---------------- Utility / Setup helpers ----------------
void setupWiFiBase() {
  // leave WiFi off for now, we'll switch to AP/STA when needed
  WiFi.mode(WIFI_OFF);
  delay(50);
  Serial.println("[init] WiFi base inactivo");
}

void setupMotores() {
  pinMode(LUCES, OUTPUT);
  pinMode(BOCINA, OUTPUT);

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

void setupMQTT() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  Serial.printf("[init] MQTT configurado (broker: %s:%d)\n", MQTT_BROKER, MQTT_PORT);
}

void setupHTTPServer() {
  // Registrar rutas UNA sola vez
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/forget", HTTP_GET, handleForget);
  server.on("/move", HTTP_POST, handleMove);

  server.onNotFound([](){
    handleRoot();
  });

  server.begin();
  Serial.println("[init] Servidor HTTP iniciado (puerto 80)");
}

// ---------------- Persistent credentials ----------------
void saveCredentials(const String &ssid, const String &pass) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
  savedSSID = ssid;
  savedPass = pass;
  Serial.println("[prefs] Credenciales guardadas.");
}

void clearCredentials() {
  preferences.begin("wifi", false);
  preferences.remove("ssid");
  preferences.remove("pass");
  preferences.end();
  savedSSID = "";
  savedPass = "";
  Serial.println("[prefs] Credenciales borradas.");
}

void loadCredentials() {
  preferences.begin("wifi", true);
  savedSSID = preferences.getString("ssid", "");
  savedPass = preferences.getString("pass", "");
  preferences.end();
  if (savedSSID != "") {
    Serial.printf("[prefs] Credencial cargada: %s\n", savedSSID.c_str());
  } else {
    Serial.println("[prefs] No hay credenciales guardadas.");
  }
}

// ---------------- AP / STA helpers ----------------
void startAPMode() {
  Serial.println("[AP] Iniciando AP modo de configuración...");
  WiFi.mode(WIFI_AP_STA);

  // Configurar IP estática del AP (opcional, pero explícito)
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  bool okCfg = WiFi.softAPConfig(local_IP, gateway, subnet);
  (void)okCfg; // no crítico

  WiFi.softAP(AP_SSID, AP_PASS);
  delay(200);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("[AP] AP IP: ");
  Serial.println(myIP);

  dnsServer.start(DNS_PORT, "*", myIP);
  Serial.println("[AP] DNS servidor iniciado para captive portal");
}

bool connectToSavedWiFi(unsigned long timeoutMs = 10000) {
  if (savedSSID == "") return false;
  Serial.printf("[WiFi] Conectando a '%s' ...\n", savedSSID.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Conectado! IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("[WiFi] No se pudo conectar con credenciales guardadas.");
    return false;
  }
}

// ---------------- MQTT reconnection ----------------
void reconnectMQTT() {
  if (mqttClient.connected()) return;
  Serial.print("[MQTT] Intentando conectar a broker...");
  unsigned long start = millis();
  while (!mqttClient.connected() && (millis() - start) < 8000) {
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("[MQTT] Conectado al broker.");
      return;
    } else {
      Serial.printf("[MQTT] falla state=%d. retry...\n", mqttClient.state());
      delay(500);
    }
  }
  Serial.println("[MQTT] No se logró conectar ahora (seguiré intentando desde loop).");
}

// ---------------- Small helpers para movimiento ----------------
void detenerMotor() {
  sonidoRev = false;
  ledcWrite(ENA_IZQ, 0);
  ledcWrite(ENA_DER, 0);
  digitalWrite(IN1_IZQ, LOW);
  digitalWrite(IN2_IZQ, LOW);
  digitalWrite(IN1_DER, LOW);
  digitalWrite(IN2_DER, LOW);
}

void acelerarMotor(int pwmVal = 818) {
  sonidoRev = false;
  digitalWrite(IN1_IZQ, HIGH); digitalWrite(IN2_IZQ, LOW);
  digitalWrite(IN1_DER, HIGH); digitalWrite(IN2_DER, LOW);
  ledcWrite(ENA_IZQ, pwmVal);
  ledcWrite(ENA_DER, pwmVal);
}

void reversa(int pwmVal = 818) {
  digitalWrite(IN1_IZQ, LOW); digitalWrite(IN2_IZQ, HIGH);
  digitalWrite(IN1_DER, LOW); digitalWrite(IN2_DER, HIGH);
  ledcWrite(ENA_IZQ, pwmVal);
  ledcWrite(ENA_DER, pwmVal);
  sonidoRev = true;
}

void izquierda(int pwmVal = 818) {
  ledcWrite(ENA_IZQ, 0);
  digitalWrite(IN1_DER, HIGH); digitalWrite(IN2_DER, LOW);
  ledcWrite(ENA_DER, pwmVal);
  sonidoRev = false;
}

void derecha(int pwmVal = 818) {
  ledcWrite(ENA_DER, 0);
  digitalWrite(IN1_IZQ, HIGH); digitalWrite(IN2_IZQ, LOW);
  ledcWrite(ENA_IZQ, pwmVal);
  sonidoRev = false;
}

void sonidoReversa() {
  if (sonando) return;
  if (sonidoRev) {
    unsigned long ahora = millis();
    if (ahora - ultimaAlarma >= intervaloBeep) {
      ultimaAlarma = ahora;
      digitalWrite(BOCINA, !digitalRead(BOCINA));
    }
  } else {
    digitalWrite(BOCINA, LOW);
  }
}

// ---------------- HTML page (unchanged) ----------------
String indexPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>ESP32 WiFi</title>
<style>body{font-family:Arial,Helvetica,sans-serif;margin:10px} .card{max-width:420px;margin:auto;padding:16px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,.12)}</style></head>
<body>
<div class="card"><h2>Configuración WiFi</h2>
<p><button onclick="doScan()">Escanear redes</button>
<button onclick="forget()">Olvidar credenciales</button></p>
<div id="networks"><p>Pulse "Escanear redes" para listar SSID.</p></div>
<div id="form" style="display:none;"><p>SSID seleccionado: <b id="ssidSelected"></b></p><input id="pwd" placeholder="Contraseña WiFi" type="password"><p><button onclick="connect()">Conectar</button></p></div>
<p id="status" style="font-weight:bold"></p>
</div>

<script>
function mkElem(tag, txt){ let e=document.createElement(tag); e.innerText=txt; return e; }
function doScan(){
  document.getElementById('networks').innerHTML = '<p>Escaneando... (3-6s)</p>';
  fetch('/scan').then(r=>r.json()).then(list=>{
    let div=document.getElementById('networks'); div.innerHTML='';
    if(list.length==0){ div.appendChild(mkElem('p','No se detectaron redes.')); return; }
    let ul=document.createElement('ul');
    list.forEach(ssid=>{ let li=document.createElement('li'); li.innerText=ssid; li.onclick=function(){ selectSSID(ssid); }; ul.appendChild(li); });
    div.appendChild(ul);
  }).catch(e=>{ document.getElementById('networks').innerHTML = '<p>Error en escaneo</p>'; });
}
function selectSSID(s){ document.getElementById('ssidSelected').innerText = s; document.getElementById('form').style.display='block'; document.getElementById('status').innerText=''; }
function connect(){ let ssid=document.getElementById('ssidSelected').innerText; let pwd=document.getElementById('pwd').value; if(!ssid){ alert('Selecciona un SSID'); return; } document.getElementById('status').innerText='Intentando conectar...';
 fetch('/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:ssid,pass:pwd})}).then(r=>r.json()).then(obj=>{ if(obj.success){ document.getElementById('status').innerText='Conectado: '+obj.ip; } else { document.getElementById('status').innerText='Error: '+obj.message; } }).catch(e=>{ document.getElementById('status').innerText='Error en petición'; }); }
function forget(){ fetch('/forget').then(r=>r.text()).then(t=>{ alert(t); location.reload(); }); }

// check status
fetch('/status').then(r=>r.json()).then(s=>{ if(s.connected) document.getElementById('status').innerText = 'Ya conectado a: ' + s.ssid + ' ('+s.ip+')'; });
</script>
</body>
</html>
)rawliteral";
  return html;
}

// ---------------- Handlers (usar ArduinoJson para parsing robusto) ----------------

void handleRoot() {
  server.send(200, "text/html", indexPage());
}

void handleScan() {
  Serial.println("[HTTP] /scan pedido - escaneando redes...");
  // borrar cache anterior y escanear
  WiFi.scanDelete();
  int n = WiFi.scanNetworks();
  Serial.printf("[WiFi] Encontradas %d redes\n", n);
  String json = "[";
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    ssid.replace("\"","");
    json += "\"" + ssid + "\"";
    if (i < n - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleConnect() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Usar POST con JSON {ssid,pass}");
    return;
  }
  String body = server.arg("plain");
  Serial.print("[HTTP] /connect body: ");
  Serial.println(body);

  // parse with ArduinoJson
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"JSON inválido\"}");
    return;
  }
  const char* ssid = doc["ssid"] | "";
  const char* pass = doc["pass"] | "";

  if (strlen(ssid) == 0) {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"SSID vacío\"}");
    return;
  }

  Serial.printf("[WiFi] Intentando conectar a '%s'...\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  unsigned long start = millis();
  bool connected = false;
  while (millis() - start < 10000) {
    if (WiFi.status() == WL_CONNECTED) { connected = true; break; }
    delay(200);
  }

  if (connected) {
  saveCredentials(ssid, pass);
  String ip = WiFi.localIP().toString();
  server.send(200, "application/json", "{\"success\":true,\"ip\":\"" + ip + "\"}");
  Serial.println("Conectado correctamente a " + String(ssid));
  Serial.println("IP: " + ip);


  // detener AP y DNS ordenadamente
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  delay(500);

  shouldRestartAfterConnect = true;
  }
  else {
    server.send(200, "application/json", "{\"success\":false,\"message\":\"No se pudo conectar con esas credenciales\"}");
  }
}

void handleStatus() {
  bool connected = (WiFi.status() == WL_CONNECTED);
  String ss = connected ? WiFi.SSID() : "";
  String ip = connected ? WiFi.localIP().toString() : "";
  String json = "{\"connected\":" + String(connected ? "true":"false") + ",\"ssid\":\"" + ss + "\",\"ip\":\"" + ip + "\"}";
  server.send(200, "application/json", json);
}

void handleForget() {
  clearCredentials();
  server.send(200, "text/plain", "Credenciales borradas. Reiniciando...");
  delay(2000);
  ESP.restart();
}

void handleMove() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Usar POST con JSON");
    return;
  }
  String body = server.arg("plain");
  String clientIp = server.client().remoteIP().toString();
  Serial.print("[HTTP] /move body: ");
  Serial.println(body);

  // parse JSON
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON inválido\"}");
    return;
  }

  const char* direction = doc["direction"] | "";
  int speed = doc["speed"] | 0;
  int duration = doc["duration"] | 0;

  if (duration > 5000) duration = 5000; // tope de seguridad

  // map speed if needed to PWM range (0..1023 or 0..2^PWM_RES_BITS - 1)
  int pwmVal = constrain(speed, 0, (1 << PWM_RES_BITS) - 1);

  // Ejecutar movimiento (bloqueante)
  if (strcmp(direction, "adelante") == 0) acelerarMotor(pwmVal);
  else if (strcmp(direction, "atras") == 0) reversa(pwmVal);
  else if (strcmp(direction, "izquierda") == 0) izquierda(pwmVal);
  else if (strcmp(direction, "derecha") == 0) derecha(pwmVal);
  else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"direction inválida\"}");
    return;
  }

  // publicar en MQTT
  JsonDocument out;
  out["ip_cliente"] = clientIp;
  out["direction"] = direction;
  out["speed"] = speed;
  out["duration"] = duration;
  String outStr;
  serializeJson(out, outStr);
  if (mqttClient.connected()) mqttClient.publish(MQTT_TOPIC_CMD, outStr.c_str());

  // mantener el movimiento el tiempo solicitado (máx 5s)
  delay(duration);
  detenerMotor();

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ---------------- Setup / Loop ----------------
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("[BOOT] Iniciando ESP32 Car");
  Serial.println("[init] Pines motores configurados");

  loadCredentials();

  setupMotores();
  startAPMode();
  setupMQTT();
  setupHTTPServer();
  sensorsSetup();
}

void loop() {
  // Solo procesar DNS si el AP está activo (evita procesar si no está iniciado)
  IPAddress apIP = WiFi.softAPIP();
  if (apIP != IPAddress(0,0,0,0)) {
    dnsServer.processNextRequest();
  }

  server.handleClient();

  // MQTT: reconexión y loop
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
    publishUltrasonicIfDue(mqttClient);
  }

  // Reinicio tras configurar /connect (flag set en handleConnect)
  if (shouldRestartAfterConnect) {
    Serial.println("[SETUP] Reiniciando para completar la conexión...");
    shouldRestartAfterConnect = false;
    delay(200);
    ESP.restart();
  }

  // Reintento de reconexión WiFi periódico (si hay credenciales)
  static unsigned long lastReconnectAttempt = 0;
  if (WiFi.status() != WL_CONNECTED && savedSSID != "") {
    if (millis() - lastReconnectAttempt > 10000) {
      Serial.println("[WiFi] Intentando reconectar a WiFi guardada...");
      lastReconnectAttempt = millis();
      if (connectToSavedWiFi(8000)) {
        Serial.println("[WiFi] Reconexión exitosa.");
      } else {
        Serial.println("[WiFi] Falló reconexión: volviendo a AP para reconfiguración.");
        startAPMode();
      }
    }
  }

  sonidoReversa();
  yield();
  delay(2);
}