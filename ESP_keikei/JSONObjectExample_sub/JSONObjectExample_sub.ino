/*
  JSON Object Example
  Simulamos la recepción de mensajes MQTT en JSON
  En la función callback():
  - se comprueba el topic
  - se intenta interpretar el mensaje en JSON
  - se busca el campo/clave "level" en el JSON
  - se obtiene el valor de la clave "level" si todo fue bien
*/
#include <ArduinoJson.h>

int ledv = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  char *mensaje=(char *)malloc(length+1); // reservo memoria para copia del mensaje
  strncpy(mensaje,(char*)payload,length); // copio el mensaje en cadena de caracteres
  mensaje[length]='\0';
  ledv = String(mensaje).toInt(); // Transforma la cadena en un entero

  Serial.print("Valor convertido: ");
  Serial.print(ledv);
  Serial.println();
  
  Serial.printf("Message arrived [%s]: %s\n",topic, mensaje);
  
  // compruebo que es el topic adecuado
  if(strcmp(topic,"infind/GRUPO0/led/cmd")==0)
  {
    StaticJsonDocument<512> root; // el tamaño tiene que ser adecuado para el mensaje
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(root, mensaje,length);

    // Compruebo si no hubo error
    if (error) {
      Serial.print("Error deserializeJson() failed: ");
      Serial.println(error.c_str());
    }
    else
    if(root.containsKey("level"))  // comprobar si existe el campo/clave que estamos buscando
    { 
     int valor = root["level"];
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

  free(mensaje); // libero memoria
}

void setup() {

  Serial.begin(115200);
  Serial.println();
  Serial.println("Comenzamos...");
  //creo cadena JSON

  Serial.print("JSON: ");
  Serial.println("85");
  
  // simulo llegada de mensajes MQTT, el primero es el correcto
  callback("infind/GRUPO0/led/cmd", (byte*)"85", strlen("85"));
  callback("otro/topic", (byte*)"85", strlen("85"));
  callback("infind/GRUPO0/led/cmd", (byte*)"level:10", 8);
  callback("infind/GRUPO0/led/cmd", (byte*)"{\"valor\":10}", 12);
   
}

void loop() {
}
