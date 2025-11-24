# CarControl_Evo -- Documento Unificado

## Integrantes de trabajo

- Danna Alejandra Sanchez Monsalve
- Juan Pablo Vargas Jimenez

---

# 1. Protocolo TLS, Certificados y Seguridad en el ESP32

## ¿Qué es el protocolo TLS y por qué es importante?

TLS (Transport Layer Security) es la versión moderna y segura de SSL.\
Su propósito es **proteger la comunicación entre dos dispositivos**
(como un navegador, un sitio web o un ESP32 y un servidor), asegurando
que los datos viajan cifrados y no pueden ser leídos ni modificados por
terceros.

---

## ¿Qué es un certificado digital?

Es un documento que demuestra que un servidor es quien dice ser.\
Incluye: - Nombre del dominio\

- Clave pública\
- Firma digital de la **Autoridad Certificadora (CA)** que comprobó su
  autenticidad

---

## Riesgos de NO usar TLS

- Los datos viajan **sin cifrar**, cualquier persona puede
  capturarlos.\
- Ataques **Man-in-the-Middle (MITM)**.\
- Pérdida de integridad: los mensajes pueden ser modificados sin que
  nadie lo note.\
- Robo de datos personales o suplantación.

---

## ¿Qué es una CA (Certificate Authority)?

Es una organización de confianza que: 1. Verifica que un dominio es
legítimo.\
2. Firma certificados digitales.

Ejemplos: Let's Encrypt, DigiCert, GlobalSign.

---

## ¿Qué es una cadena de certificados y su vigencia?

Una cadena típica incluye: 1. **Certificado raíz** -- 10 a 20 años\
2. **Certificado intermedio** -- 3 a 10 años\
3. **Certificado del dominio** -- 3 meses a 1 año

Cada eslabón certifica al siguiente.

---

## Keystore y Certificate Bundle

- **Keystore:** archivo que almacena claves privadas, públicas y
  certificados. Protegido con contraseña.\
- **Certificate bundle:** archivo que contiene varias CAs raíz e
  intermedias. Facilita la compatibilidad en clientes.

---

## ¿Qué es la autenticación mutua (mTLS)?

Es una conexión TLS donde **ambas partes** (cliente y servidor) deben
demostrar su identidad mediante certificados.\
Se usa mucho en IoT seguro.

---

## ¿Cómo habilitar la validación de certificados en el ESP32?

El ESP32 requiere que el firmware incluya el certificado raíz del
servidor:

```cpp
espClientSecure.setCACert(ROOT_CA);
```

Esto permite validar al servidor durante el handshake TLS.

ESPs pueden usar: - `esp_tls_conn_new_sync()` - `esp_tls_cfg_t`

---

## Si el ESP32 debe conectarse a múltiples dominios

Opciones: - Incluir varios certificados raíz en el firmware\

- Usar un **certificate bundle**\
- Preferir CAs ampliamente soportadas como Let's Encrypt

---

## ¿Cómo obtener un certificado?

- **Let's Encrypt** → Gratis, automático\
- CA comerciales: DigiCert, GlobalSign, Comodo

---

## ¿Qué son llave pública y privada?

TLS usa criptografía asimétrica:

Llave Función

---

**Pública** Se comparte, sirve para cifrar o verificar firmas
**Privada** Se guarda en secreto, sirve para descifrar o firmar

---

## ¿Qué pasa cuando un certificado expira?

El cliente (ESP32) **rechaza la conexión TLS**.\
Para recuperar el acceso debes: 1. Renovar el certificado en el
servidor.\
2. Si usas validación embebida → actualizar el firmware.

---

## Fundamentos matemáticos y computación cuántica

TLS se basa en: - Teoría de números\

- Factorización\
- Logaritmo discreto\
- Curvas elípticas

La computación cuántica podría romper estos sistemas mediante: -
Algoritmo de Shor\

- Algoritmo de Grover

Por eso surge la **criptografía post-cuántica**.

---

# 2. Migración del MQTT del Carro IoT hacia TLS

## Estado inicial: MQTT sin TLS (puerto 1883)

Configuración: - Broker: test.mosquitto.org\

- Puerto: 1883\
- Cliente: `WiFiClient`\
- Librería MQTT: PubSubClient

Funcionamiento: - Conexión estable en modo AP/STA\

- Suscripción a `carro/instrucciones`\
- Publicación en `carro/telemetria/distancia`\
- Control completo vía MQTT Explorer

---

# 3. Fase 1 -- Cambio al puerto seguro 8883 sin activar TLS

Solo se modificó:

```cpp
#define MQTT_PORT 8883
```

Problema: - El cliente seguía siendo `WiFiClient` (sin TLS) -
Resultado:\
**FALLO rc = -4 → MQTT_CONNECTION_TIMEOUT**

El broker esperaba un **handshake TLS**, pero el ESP32 no lo envió.

Conclusión:\
\> Un cliente sin TLS jamás podrá conectarse al puerto 8883.

---

# 4. Fase 2 -- Activación de TLS sin validar certificados

Se reemplazó:

```cpp
WiFiClient espClient;
```

por:

```cpp
WiFiClientSecure espClientSecure;
espClientSecure.setInsecure();
```

Resultados: - El canal se cifra\

- El broker acepta la conexión\
- No se valida al servidor → vulnerable a MITM

---

# 5. Comportamiento en MQTT Explorer

- Con puerto 8883 sin TLS → **NO se conecta**\
- Para activar TLS se debe usar:\
  mqtt:// + **Encryption (tls) ON**

---

# 6. Fase 3 -- Activación de validación de certificados

Se activó:

```cpp
espClientSecure.setCACert(ROOT_CA);
```

Si el CA es incorrecto: - Error TLS: **X509 - Certificate verification
failed (código -9984)**\

- PubSubClient produce: **rc = -2**

Esto demuestra que la validación funciona.

Para test.mosquitto.org se requiere cargar: - `mosquitto.org.crt` (CA
raíz oficial)

---

# 7. Fase 4 -- Migración final hacia AWS IoT Core

Se configuró: - Endpoint:\
`a2hfirfjvweeu1-ats.iot.us-east-1.amazonaws.com` - Puerto seguro:
**8883** - Certificados generados: - Certificado X.509 del dispositivo\

- Clave privada\
- Amazon Root CA 1

En el código:

```cpp
espClientSecure.setCACert(AWS_ROOT_CA);
espClientSecure.setCertificate(DEVICE_CERT);
espClientSecure.setPrivateKey(DEVICE_KEY);
```

La conexión se estableció correctamente y AWS recibió: - Telemetría del
carro\

- Distancias del sensor ultrasónico\
- Estados de movimiento y luces

---

# 8. Bibliografía Consolidada

- https://youtu.be/AKG-rljqaDE\
- https://youtu.be/vwvtsdSxeq8\
- https://youtu.be/qnYUjl9sirU\
- Explicación de la cadena de confianza de los certificados -- SSL
  Dragon\
- ¿Qué es TLS? \| TLS mutuo \| Cloudflare\
- https://cursos.asimov.mx/curso_esp32/modulo_9/https-y-certificados-ssl-en-esp32.html\
- https://repository.unad.edu.co/jspui/bitstream/10596/28230/6/ilovepd.pdf

---
