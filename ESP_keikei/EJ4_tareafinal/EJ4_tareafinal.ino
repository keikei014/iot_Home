#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#ifdef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP8266 ONLY!)
#error Select ESP8266 board.
#endif

DHTesp dht;

char mensaje[128]; // variable en la que guardar el mensaje en formato json

// datos de la conexión a internet y a MQTT
const char* ssid = "MiFibra-2CE2_plus";
const char* password = "ejN2bYJT";
const char* mqtt_server = "192.168.1.87";
//const char* ssid = "infind";
//const char* password = "1518wifi";
//const char* mqtt_server = "192.168.1.159";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200); // Inicializa el puerto serie
  Serial.println(); // Línea en blanco
  
  dht.setup(5, DHTesp::DHT11); // Connect DHT sensor to GPIO 5

  setup_wifi();
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

  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    delay(dht.getMinimumSamplingPeriod());

    float hum = dht.getHumidity(); // Recibe el dato de humedad y lo guarda en hum
    float temp = dht.getTemperature(); // Recibe el dato de temperatura y lo guarda en temp

    snprintf(mensaje, 128, "{\"temperatura\": %f, \"humedad\": %f}", temp, hum); // Construyo el mensaje en formato json

    Serial.print(dht.getStatusString());
    Serial.print("\t");
    Serial.print(hum, 1);
    Serial.print("\t\t");
    Serial.print(temp, 1);
    Serial.print("\t\t");
    Serial.print(dht.toFahrenheit(temp), 1);
    Serial.print("\t\t");
    Serial.print(dht.computeHeatIndex(temp, hum, false), 1);
    Serial.print("\t\t");
    Serial.println(dht.computeHeatIndex(dht.toFahrenheit(temp), hum, true), 1);
    delay(2000);

    unsigned long now = millis();
    
    lastMsg = now;
    Serial.print("Publish message: ");
    Serial.println(mensaje);
    client.publish("infind/Francisco_Anguita/temperatura", mensaje);
  }
}
