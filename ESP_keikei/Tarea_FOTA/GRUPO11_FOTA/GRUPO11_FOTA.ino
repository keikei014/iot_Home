#include "DHTesp.h"
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

DHTesp dht; // Para usar el sensor

ADC_MODE(ADC_VCC); // Permite lectura de la alimentación del sensor

// datos para actualización   
#define HTTP_OTA_ADDRESS      F("172.16.53.###")         // Address of OTA update server
#define HTTP_OTA_PATH         F("/esp8266-ota/update") // Path to update firmware
#define HTTP_OTA_PORT         1880                     // Port of update server
                                                       // Name of firmware
#define HTTP_OTA_VERSION      String(__FILE__).substring(String(__FILE__).lastIndexOf('\\')+1) + ".nodemcu" 

char mensaje1[16]; // variables para los mensajes formateados en JSON
char mensaje2[256];
char mensaje3[16];

// datos de la conexión a internet y a MQTT
const char* ssid = "MiFibra-2CE2_plus";
const char* password = "ejN2bYJT";
const char* mqtt_server = "192.168.1.89";
//const char* ssid = "infind";
//const char* password = "1518wifi";
//const char* mqtt_server = "192.168.1.158";

// GPIOs
int LED_blink= 2;  
int LED_OTA = 16; 
// funciones para progreso de OTA
void progreso_OTA(int,int);
void final_OTA();
void inicio_OTA();
void error_OTA(int);


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
int ledv = 0; // Variable para controlar el led de GPIO2
int PWM = 0; // Variable para guardar el valor de PWM asociado a la entrada de ledv
bool estado = false; // Variable para mensaje retenido y LWT del estado de conexión

//---------------------------------------------------------------------
void intenta_OTA()
{ 
  Serial.println( "--------------------------" );  
  Serial.println( "Comprobando actualización:" );
  Serial.print(HTTP_OTA_ADDRESS);Serial.print(":");Serial.print(HTTP_OTA_PORT);Serial.println(HTTP_OTA_PATH);
  Serial.println( "--------------------------" );  
  ESPhttpUpdate.setLedPin(LED_OTA, LOW);
  ESPhttpUpdate.onStart(inicio_OTA);
  ESPhttpUpdate.onError(error_OTA);
  ESPhttpUpdate.onProgress(progreso_OTA);
  ESPhttpUpdate.onEnd(final_OTA);
  WiFiClient wClient;
  switch(ESPhttpUpdate.update(wClient, HTTP_OTA_ADDRESS, HTTP_OTA_PORT, HTTP_OTA_PATH, HTTP_OTA_VERSION)) {
    case HTTP_UPDATE_FAILED:
      Serial.printf(" HTTP update failed: Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F(" El dispositivo ya está actualizado"));
      break;
    case HTTP_UPDATE_OK:
      Serial.println(F(" OK"));
      break;
    }
}

//-----------------------------------------------------
void final_OTA()
{
  Serial.println("Fin OTA. Reiniciando...");
}

void inicio_OTA()
{
  Serial.println("Nuevo Firmware encontrado. Actualizando...");
}

void error_OTA(int e)
{
  char cadena[64];
  snprintf(cadena,64,"ERROR: %d",e);
  Serial.println(cadena);
}

void progreso_OTA(int x, int todo)
{
  char cadena[256];
  int progress=(int)((x*100)/todo);
  if(progress%10==0)
  {
    snprintf(cadena,256,"Progreso: %d%% - %dK de %dK",progress,x/1024,todo/1024);
    Serial.println(cadena);
  }
}

//-----------------------------------------------------

void setup_wifi() {
  
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(ESP.getChipId());
    // Attempt to connect adding a LWT
    StaticJsonDocument<8> doc;
    doc["online"] = estado;
    serializeJson(doc, mensaje1);
    // boolean connect(clientId.c_str(),[NULL,NULL],["infind/GRUPO11/conexion",1,true, mensaje1],[true])
    if (client.connect(clientId.c_str(),"infind/GRUPO11/conexion",2,true,mensaje1)) 
    {
      estado = true;
      Serial.println("connected");
      // Al conectarse, lo notifica y publica el estado online de la conexion
      client.publish("outTopic", "GANGASTAAAS wassup guys");
      
      //Publicar el mensaje retenido con el estado de la conexión
      StaticJsonDocument<16> doc;
      doc["online"] = true;
      serializeJson(doc,mensaje1);
      
      client.publish("infind/GRUPO11/conexion", mensaje1,true);
      // Se suscribe a inTopic (en este caso no hace falta) y al dato del led de nuestro NODERed
      client.subscribe("inTopic");
      client.subscribe("infind/GRUPO11/led/cmd");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length){
  
  char* cadena = (char*)malloc(length+1); // Reservo memoria para copiar el mensaje
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  strncpy(cadena,(char*)payload,length); // Copia del mensaje en la vble cadena
  cadena[length]='\0'; // Esto no se para que es pero padentro
  
  // compruebo que es el topic adecuado
  if(strcmp(topic,"infind/GRUPO11/led/cmd")==0)
  {
    StaticJsonDocument<512> root; // el tamaño tiene que ser adecuado para el mensaje
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(root, cadena,length);

    // Compruebo si no hubo error
    if (error) {
      Serial.print("Error deserializeJson() failed: ");
      Serial.println(error.c_str());
    }
    else
    if(root.containsKey("level"))  // comprobar si existe el campo/clave que estamos buscando
    { 
     int valor = root["level"];
     ledv = valor; // Guarda el valor del campo 'level' en la vble del mensaje que se publica
     Serial.print("Mensaje OK, level = ");
     Serial.println(valor);
    }
    else
    {
      Serial.print("Error : ");
      Serial.println("\"level\" key not found in JSON");
    }
  } // if topic
  else
  {
    Serial.println("Error: Topic desconocido");
  }

  free(cadena); // libero memoria

  
  PWM = (100-ledv)*255/100; // Esta expresión transforma al rango PWM (lógica negativa del PIN)

  StaticJsonDocument<16> doc1;
  doc1["led"] = ledv;
  serializeJson(doc1, mensaje3);

  Serial.print("Publish message: ");
  Serial.println(mensaje3);
  client.publish("infind/GRUPO11/led/status", mensaje3);
}

void setup() {
  
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200); // Inicializa el puerto serie
  Serial.println(); // Línea en blanco
  
  dht.setup(5, DHTesp::DHT11); // Connect DHT sensor to GPIO 5

  setup_wifi();
  intenta_OTA();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  Serial.println("Todo Setupeado"); // Fin del setup

}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  

  analogWrite(BUILTIN_LED,PWM);
  
  unsigned long now = millis();
  if (now - lastMsg > 30000) {
    delay(dht.getMinimumSamplingPeriod());

    float VCC = ESP.getVcc(); // Recibe el dato de la batería y lo guarda en VCC
    float temp = dht.getTemperature();
    float hum = dht.getHumidity();


    StaticJsonDocument<256> doc;

    doc["Uptime"] = now;
    doc["Vcc"] = VCC/1000;

    JsonObject DHT11 = doc.createNestedObject("DHT11");
    DHT11["temperatura"] = temp;
    DHT11["humedad"] = hum;
    doc["LED"] = ledv;

    JsonObject Wifi = doc.createNestedObject("Wifi");
    Wifi["SSID"] = ssid;
    Wifi["IP"] = WiFi.localIP();
    Wifi["RSSI"] = WiFi.RSSI();

    serializeJson(doc, mensaje2);
    
    unsigned long now = millis();
    
    lastMsg = now;
    Serial.print("Publish message: ");
    Serial.println(mensaje2);
    // Aquí estaba la publicación de mensaje1 pero me la he llevado al reconnect
    client.publish("infind/GRUPO11/datos", mensaje2);
    
  }
  
}
