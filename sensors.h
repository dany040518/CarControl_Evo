#pragma once
#include <PubSubClient.h>

// Inicializa pines (o semilla aleatoria) según modo
void sensorsSetup();

// Lee distancia en cm (sin argumentos). Puede ser NAN si no hay lectura válida.
float readUltrasonicCM();

// Publica periódicamente (si toca) al tema MQTT de telemetría.
// Llamar en loop().
void publishUltrasonicIfDue(PubSubClient& mqtt);