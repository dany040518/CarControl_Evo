# CarControl_Evo

## Integrantes de trabajo
- Danna Alejandra Sanchez Monsalve
- Juan Pablo Vargas Jimenez

### Protocolo TLS y Certificados
-	¿Qué es el protocolo TLS, cual es su importancia y que es un certificado en ese contexto?

Seguridad de la capa de transporte, es una versión mejorada del SSL ya que brinda mejores características de seguridad y su función es protegiendo la comunicación entre 2 dispositivos en internet ya sea tu navegado, pagina web o ESP32 y un servidor cifrando la información para que nadie pueda leerla ni modificarla mientras es transportada
Un certificado es una credencial que demuestra que ese sitio es quien dice ser, el certificado contiene el nombre del dominio, la clave publica y la firma de la autoridad que dio el certificado

- ¿A que riesgos se está expuesto si no se usa TLS?

La información viajara sin ser cifrada y cualquiera podria leerla, riesgo de un ataque “man-in-the-middle” donde alguien se haga pasar por el servidor, aquel usuario que este usando el sitio puede ser enganado para enviar datos personales, y los datos pueden perder su integrdad al ser modificados sin que nadie lo note.

-	¿Qué es un CA (Certificate Authority)?

Una CA (Autoridad Certificadora) es una organización de confianza que emite certificados digitales. Verifica que quien pide el certificado realmente posee el dominio y luego firma digitalmente el certificado.

-	¿Qué es una cadena de certificados y cuál es la vigencia promedio de los eslabones de la cadena?

Es una secuencia de certificados, cada uno de los cuales certifica al anterior, empezando por el certificado raíz de confianza, pasando por el certificado intermedio y terminando por el certificado del sitio web, su vigencia es de aproximadamente:
  - Certificado raíz: 10–20 años
  - Intermedio: 3–10 años
  - Certificado del dominio (sitio web): 3 meses a 1 año

-	¿Que es un keystore y que es un certificate bundle?

Keystore es un archivo donde se guardan claves privadas, públicas y certificados. Se usa para que un sistema pueda identificarse de forma segura y se protege mediante contraseña para garantizar que lo allí almacenado este seguro y solo aquellos con autorización puedan acceder.
Certificate bundle es un archivo que contiene los certificados que completan la cadena de certificados y es esencial para mejorar la compatibilidad de los certificados con navegadores web, clientes de correo electrónico y dispositivos móviles.

-	¿Qué es la autenticación mutua en el contexto de TLS?

mTLS o TLS mutuo garantiza que las partes de cada extremo de una conexión de red son quienes dicen ser, verificando que ambas tienen la clave privada correcta

-	¿Cómo se habilita la validación de certificados en el ESP32?


-	Si el sketch necesita conectarse a múltiples dominios con certificados generados por CAs distintos, ¿que alternativas hay?
-	¿Cómo se puede obtener el certificado para un dominio?
-	¿A qué se hace referencia cuando se habla de llave publica y privada en el contexto de TLS?
-	¿Que pasará con el código cuando los certificados expiren?
-	¿Que teoría matemática es el fundamento de la criptografía moderna? ¿Cuales son las posibles implicaciones de la computación cuántica para los métodos de criptografía actuales?
