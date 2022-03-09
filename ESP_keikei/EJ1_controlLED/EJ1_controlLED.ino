int ledState = LOW;

unsigned long previousMillis = 0;
const long interval = 1000;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Todo setupeado");
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (ledState == LOW) {
      ledState = HIGH;  // Note that this switches the LED *off*
      Serial.println("APAGADO");
    } else {
      ledState = LOW;  // Note that this switches the LED *on*
      Serial.println("ENCENDIDO");
    }
    digitalWrite(LED_BUILTIN, ledState);
  }
}
