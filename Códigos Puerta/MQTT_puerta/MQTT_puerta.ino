#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// servidor y punto de acceso al que se va a conectar la placa
const char* ssid = "MiFibra-2CE2_plus";
const char* password = "ejN2bYJT";
const char* mqtt_server = "iot.ac.uma.es";
const char* user = "II11";
const char* mqtt_password = "jzWeZUIo";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

// variables para almacenar mensajes en JSON
char LWT[16];
char retenido[16];
char msg_datos[256];
char msg_puerta[256];

// variables incluidas en los mensajes JSON
boolean abierta = false;
boolean seguridad = false;
int modo = 1;
int T_envio = 30000;
int T_actualizacion = 300000;
int lastAct;


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

    // mensaje JSON para el Last Will Testament
    StaticJsonDocument<16> doc;
    doc["Chip ID"] = ESP.getChipId();
    doc["online"] = false;
    serializeJson(doc,LWT);
    
    // Se intenta la conexión
    if ( client.connect(clientId.c_str()), user, mqtt_password, "II11/ESP1066766/conexion",2,true,LWT ) {
      Serial.println("connected"); // informa de que la conexión ha sido exitosa

      // topics a los que se suscribe la placa que controla la puerta
      client.subscribe("II11/ESP1066766/config");
      client.subscribe("II11/ESP1066766/puerta");

      // formatear el mensaje retenido que se va a publiar
      StaticJsonDocument<16> doc;
      doc["Chip ID"] = ESP.getChipId();
      doc["online"] = true;
      serializeJson(doc,retenido);
      
      // publica el mensaje retenido de conexion
      client.publish( "II11,ESP1066766/conexion", retenido, true );
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Espera los 5 segundos
      delay(5000);
    }
  }
}

void callback( char* topic, byte* payload, unsigned int length ) {

  char* cadena = (char*)malloc(length+1); // Reserva de memoria para copiar el mensaje recibido
  // Escribe en consola el topic del mensaje recibido y se imprime su contenido
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Se copia el mensaje almacenado en payload en la variable cadena
  strncpy(cadena,(char*)payload,length); 
  cadena[length]='\0';

  // Se comprueba el topic recibido y se actúa en consecuencia
  if( strcmp(topic,"II11/ESP1066766/config") == 0 ) {
    Serial.println("Me ha llegao una configuracion to wapa");
  } else if( strcmp(topic,"II11/ESP1066766/puerta") == 0 ) {
    Serial.println("Me ha llegao una puerta to flama");
  }
}

//------------------------------------------FIN MQTT-------------------------------------------

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // Inicializa el puerto serie
  Serial.println(); // Línea en blanco

  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
  
  Serial.println("Todo Setupeado"); // Fin del setup
}

void loop() {
  // put your main code here, to run repeatedly:
    // no hará nada hasta que logre conectarse a MQTT
    if (!client.connected()) {
      reconnect();
    }
    client.loop(); 

    unsigned long now = millis();
    if( now - lastMsg > T_envio ) {

      // Crea el mensaje de datos
      StaticJsonDocument<256> doc_datos;

      doc_datos["Chip ID"] = ESP.getChipId();
      doc_datos["Uptime"] = millis();
      doc_datos["envio"] = T_envio;
      doc_datos["actualizacion"] = T_actualizacion;

      JsonObject Wifi = doc_datos.createNestedObject("Wifi");
      Wifi["SSID"] = ssid;
      Wifi["IP"] = WiFi.localIP();
      Wifi["RSSI"] = WiFi.RSSI();

      serializeJson(doc_datos, msg_datos);

      // Crea el mensaje de infor sobre estado de la puerta
      StaticJsonDocument<256> doc;

      doc["modo"] = modo;
      doc["seguridad"] = seguridad;
      doc["abierta"] = abierta;

      serializeJson(doc, msg_puerta);

      // se publican los mensajes y se actualiza lastMsg
      lastMsg = now;

      Serial.print("Publish message: ");
      Serial.println(msg_datos);
      client.publish("II11/ESP1066766/datos", msg_datos);

      Serial.print("Publish message: ");
      Serial.println(msg_puerta);
      client.publish("II11/ESP1066766/estado_puerta", msg_puerta);
    }

    if( now - lastAct > T_actualizacion ) {
      Serial.println("Ahora tocaria actualizar");
      lastAct = now;
    }
    
}
