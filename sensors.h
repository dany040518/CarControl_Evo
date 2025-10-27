#pragma once
#include <PubSubClient.h>

// Inicializa pines
void sensorsSetup();

// Lee distancia en cm
float readUltrasonicCM();

// Publica periódicamente
void publishUltrasonicIfDue(PubSubClient& mqtt);
