/*
 Basic ESP8266 MQTT example
 
 Principales diferencias con el ejemplo de la librería PubSubClient:
   Utiliza el ID del chip en setup() para:
     - construir un identificador único para conectar con MQTT (ESP_???????)
     - construir topics únicos para esta placa (infind/ESP_???????/+)
   Conecta a MQTT usando usuario/contraseña (conecta_mqtt())
   Configura el servidor MQTT para admitir mensajes hasta 512 bytes (setup())
   En callback():
     - copia el mensaje a una cadena de caracteres (gestión de memoria con malloc/free)
     - comprueba el valor del topic
   Al enviar un mensaje enciende el LED correspondiente al GPIO16
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

WiFiClient wClient;
PubSubClient mqtt_client(wClient);

// Update these with values suitable for your network.
const char* ssid = "infind";
const char* password = "1518wifi";
const char* mqtt_server = "iot.ac.uma.es";
const char* mqtt_user = "infind";
const char* mqtt_pass = "zancudo";

// cadenas para topics e ID
char ID_PLACA[16];
char topic_PUB[256];
char topic_SUB[256];

// GPIOs
int LED1 = 2;  
int LED2 = 16; 

//-----------------------------------------------------
void conecta_wifi() {
  Serial.printf("\nConnecting to %s:\n", ssid);
 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected, IP address: %s\n", WiFi.localIP().toString().c_str());
}

//-----------------------------------------------------
void conecta_mqtt() {
  // Loop until we're reconnected
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt_client.connect(ID_PLACA, mqtt_user, mqtt_pass)) {
      Serial.printf(" conectado a broker: %s\n",mqtt_server);
      mqtt_client.subscribe(topic_SUB);
    } else {
      Serial.printf("failed, rc=%d  try again in 5s\n", mqtt_client.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//-----------------------------------------------------
void procesa_mensaje(char* topic, byte* payload, unsigned int length) {
  char *mensaje = (char *)malloc(length+1); // reservo memoria para copia del mensaje
  strncpy(mensaje, (char*)payload, length); // copio el mensaje en cadena de caracteres
  mensaje[length]='\0'; // caracter cero marca el final de la cadena
  Serial.printf("Mensaje recibido [%s] %s\n", topic, mensaje);
  // compruebo el topic
  if(strcmp(topic, topic_SUB)==0) 
  {
    if (mensaje[0] == '1') {
        digitalWrite(LED1, LOW);   // Turn the LED on (Note that LOW is the voltage level 
      } else {
        digitalWrite(LED1, HIGH);  // Turn the LED off by making the voltage HIGH
      }
  }
  free(mensaje);
}

//-----------------------------------------------------
//     SETUP
//-----------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Empieza setup...");
  pinMode(LED1, OUTPUT);    // inicializa GPIO como salida
  pinMode(LED2, OUTPUT);    
  digitalWrite(LED1, HIGH); // apaga el led
  digitalWrite(LED2, HIGH); 
  // crea topics usando id de la placa
  sprintf(ID_PLACA, "ESP_%d", ESP.getChipId());
  sprintf(topic_PUB, "infind/%s/pub", ID_PLACA);
  sprintf(topic_SUB, "infind/%s/sub", ID_PLACA);
  conecta_wifi();
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setBufferSize(512); // para poder enviar mensajes de hasta X bytes
  mqtt_client.setCallback(procesa_mensaje);
  conecta_mqtt();
  Serial.printf("Identificador placa: %s\n", ID_PLACA );
  Serial.printf("Topic publicacion  : %s\n", topic_PUB);
  Serial.printf("Topic subscripcion : %s\n", topic_SUB);
  Serial.printf("Termina setup en %lu ms\n\n",millis());
}

//-----------------------------------------------------
#define TAMANHO_MENSAJE 128
unsigned long ultimo_mensaje=0;
//-----------------------------------------------------
//     LOOP
//-----------------------------------------------------
void loop() {
  if (!mqtt_client.connected()) conecta_mqtt();
  mqtt_client.loop(); // esta llamada para que la librería recupere el control
  unsigned long ahora = millis();
  if (ahora - ultimo_mensaje >= 5000) {
    char mensaje[TAMANHO_MENSAJE];
    ultimo_mensaje = ahora;
    snprintf (mensaje, TAMANHO_MENSAJE, "Mensaje enviado desde %s en %6lu ms", ID_PLACA, ahora);
    Serial.println(mensaje);
    mqtt_client.publish(topic_PUB, mensaje);
    digitalWrite(LED2, LOW); // enciende el led al enviar mensaje
  }
  if (digitalRead(LED2)==LOW && ahora-ultimo_mensaje>=100) 
    digitalWrite(LED2, HIGH); 
}
