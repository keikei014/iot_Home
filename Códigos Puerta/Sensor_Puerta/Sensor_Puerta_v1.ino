// de giro del motor (https://www.youtube.com/watch?v=sL2vyUCrcEo&list=PLG8UtYUFOQj7cgO1wh2wPFF1pobyURI3_&index=15)

// Aquí viene bien explicao cómo van los final de carrera 
//(https://www.google.com/search?client=opera-gx&q=arduino+sensor+fin+de+carrera&sourceid=opera&ie=UTF-8&oe=UTF-8#kpvalbx=_TB_CYfveLNGua-vSh4AN32)

#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

// DATOS PARA LA ACTUALIZACIÓN MEDIANTE FOTA 
#define HTTP_OTA_ADDRESS      F("192.168.0.14")         // Address of OTA update server
#define HTTP_OTA_PATH         F("/esp8266-ota/update") // Path to update firmware
#define HTTP_OTA_PORT         1880                     // Port of update server
                                                       // Name of firmware
#define HTTP_OTA_VERSION      String(__FILE__).substring(String(__FILE__).lastIndexOf('\\')+1) + ".nodemcu"

// variables para los mensajes formateados en JSON (CAMBIAR)
char mensaje1[16]; 
char mensaje2[256];
char mensaje3[16];

// datos de la conexión a internet y a MQTT 
const char* ssid = "vodafoneBA1712";
const char* password = "35PCX7JFFJ7GDEC5";
const char* mqtt_server = "192.168.0.14";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

// FUNCIONES PARA EL PROCESO DE ACTUALIZACIÓN FOTA
void progreso_OTA(int,int);
void final_OTA();
void inicio_OTA();
void error_OTA(int);

// VARIABLES PARA EL FUNCIONAMIENTO DEL MOTOR
const int P_Sensor = 2; // Pin al que llegan datos del sensor de presencia
const int P_Motor = 14; // Pin por el que se envía actuación al motor
const int Cerrado = 3; // Pin al que está conectado el fdc de cerrado
const int Abierto = 4; // Pin al que está conectado el fdc de abierto
const int LED_OTA = 16; // Led que parpadea durante el proceso de actualización
boolean sensorValue = LOW; // Vble para guardar la lectura del sensor
boolean fdc_C = LOW; // Vble para guardar lectura del fdc de cerrado
boolean fdc_A = LOW; // Vble para guardar lectura del fdc de abierto


//------------------------------------------- ACTUALIZACIÓN FOTA -----------------------------
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

// FUNCIONES AUXILIARES A FOTA
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

//---------------------------------------FIN FOTA--------------------------------------------

//------------------------------------CONFIGURAR WIFI-----------------------------------------

void setup_wifi() {

  delay(10);

  // Se muestra la red a la que está intentando conectarse
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid,password); // intenta la conexión

  // escribe puntos como señal de que está intentando la conexión
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  // Una vez lograda la conexión, devuelve la dirección IP local
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP adress: ");
  Serial.println(WiFi.localIP());
  
}
//---------------------------------------FIN WIFI ---------------------------------------------

//------------------------------------CONECTAR A MQTT----------------------------------------

void reconnect() {

  // Bucle mientras no se logre una conexión exitosa
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Se crea el ID del cliente
    String clientId = "ESP8266Client-" + String(ESP.getChipId()); 
    // Se intenta la conexión
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("proyectoIoT/prueba");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Espera los 5 segundos
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {

    // Imprime el mensaje redibido y el topic por el que se ha recibido
    char* cadena = (char*)malloc(length+1);
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();

    // Copia del mensaje en la vble cadena
    strncpy(cadena,(char*)payload,length); 
    cadena[length]='\0';
}

//-----------------------------------------FIN MQTT------------------------------------------


void setup() {
  // put your setup code here, to run once:
  pinMode(P_Sensor, INPUT); // Señal del sensor como entrada
  pinMode(Cerrado, INPUT); // fdc como entrada
  pinMode(Abierto, INPUT); // fdc como entrada

  Serial.begin(115200); // Inicializa el puerto serie
  Serial.println(); // Línea en blanco

  setup_wifi(); // Se realiza la conexión a una red WiFi llamando a la función encargada de hacerlo
  intenta_OTA(); // Una vez conectado a internet, se comprueba si hay actualización y la realiza
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println(ESP.getChipId());
  Serial.println("Todo listo"); // Fin del setup, programa listo para funcionar con normalidad
}

void loop() {
  // put your main code here, to run repeatedly:
  sensorValue = digitalRead(P_Sensor); // Lee el sensor
  fdc_C = digitalRead(Cerrado); // Lee el fdc de cerrado
  fdc_A = digitalRead(Abierto); // Lee el fdc de abierto

  // REALIZA LA CONEXIÓN A MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); 
  
  // FUNCIONAMIENTO DEL MOTOR DE LA PUERTA
  if( sensorValue == HIGH and Cerrado == HIGH )
  {
    while( Abierto == LOW )
    {
      analogWrite(P_Motor,50); // Mueve el motor hasta alcanzar el fdc que indica que está abierta
    }
  }
  else if( sensorValue == LOW and Abierto == HIGH )
  {
    while( Cerrado == LOW ) // Mueve el motor hasta alcanzar el fdc que indica que está cerrada
    {
      analogWrite(P_Motor,50);
    }
  }
}
