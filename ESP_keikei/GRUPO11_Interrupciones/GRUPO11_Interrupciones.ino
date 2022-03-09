// -------------------------------------------------------------------
// Ejemplo de contol de un pulsador mediante polling e interrupciones
// -------------------------------------------------------------------

int boton_flash=0;       // GPIO0 = boton flash
int estado_int=HIGH;     // por defecto HIGH (PULLUP). Cuando se pulsa se pone a LOW

unsigned long ultima_int = 0;
volatile unsigned long ahora = 0;
volatile bool pulsado = false;
volatile bool soltado = false;
volatile unsigned long time1 = 0;

/*------------------  RTI  --------------------*/
// Rutina de Tratamiento de la Interrupcion (RTI)
ICACHE_RAM_ATTR void RTI() {
  int lectura=digitalRead(boton_flash);
  ahora = millis();
  if(lectura==estado_int || ahora-ultima_int<50) return; // filtro rebotes 50ms
  if(estado_int == HIGH){
    time1 = ahora;
    estado_int = LOW;
    pulsado = true;
  }
  else{
    estado_int = HIGH;
    soltado = true;
  }   
  ultima_int = ahora;
}

/*------------------ SETUP --------------------*/
void setup() {
  pinMode(boton_flash, INPUT_PULLUP);
  Serial.println("Pulsador listo...");
  attachInterrupt(digitalPinToInterrupt(boton_flash), RTI, CHANGE);
  Serial.println("[Setup]  Interrupción preparada...");
  Serial.begin(115200);
  Serial.println();
  
}

/*------------------- LOOP --------------------*/
void loop() { 
    if(pulsado == true){
      Serial.print("[loop]  Se detectó una pulsación en : ");
      Serial.print(ahora);
      Serial.print(" ms.");
      pulsado = false;
    }
    else if(soltado == true){
      Serial.print("[loop]  Terminó la pulsación, duración : ");
      Serial.print(millis()-time1);
      Serial.print(" ms.");
      Serial.println();
      soltado = false;
      ahora = 0;
    }
  delay(10);
}
