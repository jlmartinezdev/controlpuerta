#define ABRIENDO 0
#define CERRANDO 1
#define CERRADO 0
#define ABIERTO 1
#define IS_WORKING 2
#define VEL_MAX 255     //230
#define VEL_FRENADO 20  //10
#define SI true
#define NO false

int vel_media = 180;
int vel_minima = 80;

const int direccion_giro_out = 3;  //giro 1 o 0 en este
const int PWM_out = 11;  // PWM


/* PIN Sensores */
const int sensor1_in = 8;
const int sensor2_in = 7;


/* PIN Pulsadores */
const int pulAbierto_in = 5;
const int pulCerrado_in = 6;

const int encoderPin = 2;

/* Variable estado */
int dis_total_pulsos = 281;    //
int dis_recorrido_pulsos = 0;  // Captura distancia cuando la puerta se cierra y se activa sensor de presencia
const int tEspera = 2;         // Segundo de espera para cerrar puerta
int intento = 0;
bool sen_isActivado = false;
bool sen_isDisable = false;
bool pulAbrir_isEnable = false;
bool pulCerrar_isEnable = false;
bool isAvance = true;
byte state_puerta = CERRADO;
byte state_direccion = CERRANDO;
bool state_calibracion;
int velActual = 0;
bool flag = false;  //Variable para detectar giro al no usar attachInterrupt
long t = 0;
volatile float pulsos = 0;

long tiempo;

bool buscar_posFinal= true; // Para prueba se puede desactivar


void setup() {
  pinMode(direccion_giro_out, OUTPUT);
  pinMode(sensor1_in, INPUT);
  pinMode(sensor2_in, INPUT);
  // pinMode(encoderPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(encoderPin), increment, FALLING);  // Contar pulso del motor
  // pinMode(senFin, INPUT);
  // pinMode(senInicio, INPUT);
  // pinMode(pulAbierto, INPUT);
  // pinMode(pulCerrado, INPUT);
  Serial.begin(115200);
  detenerMotor(NO);
  delay(500);
  Serial.println("Iniciando Sistema...");
  buscarPosFinal(true, CERRANDO, 30); 
}



//ICACHE_RAM_ATTR
void increment() {
  pulsos++;
}

void buscarPosFinal(boolean close, byte dir, byte velocidad) {
  if(!buscar_posFinal){
    return;
  }
  Serial.println("Buscar posicion final ");
  Serial.println(dir == ABRIENDO ? "ABRIENDO" : "CERRANDO");
  byte estado = ABIERTO;
  int pul = 0;
  long now;
  setDireccionGiro(dir);
  girarMotor(velocidad);
  delay(500);

  t = millis();  //Marcar tiempo para comparar
  do {
    now = millis();
    if (now - t > 350) { // Revisar cada 350ms
      if (pulsos == pul) {  // Cuando el encoder deja de girar
        Serial.print("Pulso encoder: ");
        Serial.print(pulsos);
        Serial.print("Pulso buscando pos final : ");
        Serial.println(pul);
        estado = CERRADO;
      }
      t = now;
      pul = pulsos;
    }
  } while (estado == ABIERTO);

  detenerMotor(NO);

  if (close) {
    if (state_direccion == ABRIENDO) {
      state_puerta = ABIERTO;  // Finaliza proceso puerta abierta
    } else {
      state_puerta = CERRADO;  //Finaliza proceso puerta cerrada
    }
    t = millis();           // Resetea tiempo espera
    sen_isDisable = false;  // Activa Sensor para detectar presencia
  }

}
/**
 * @brief Estable direccion
 * 
 * @param direccion (ABRIENDO- CERRANDO)
 */
void setDireccionGiro(byte direccion) {
  state_direccion = direccion;
  digitalWrite(direccion_giro_out, direccion == ABRIENDO ? HIGH : LOW);
  Serial.print("Se establecio direccion de giro:");
  Serial.println(direccion==ABRIENDO ? "ABRIENDO.." : "CERRANDO...");
}
/**
 * @brief Salida PWM
 * 
 * @param velocidad (0-255)
 */
void girarMotor(byte velocidad) {
  analogWrite(PWM_out, velocidad);
}



void detenerMotor(boolean frenar) {
  Serial.print("Detener Motor se aplica frenado:");
  Serial.println(frenar ? "SI": "NO");
  if (frenar) {
    frenarMotor();
  }
  analogWrite(PWM_out, 0);
}
void frenarMotor() {
  //Aplicar frenado
  byte stado = 0;
  int c = dis_recorrido_pulsos;
  girarMotor(0);
  do {
    if (c != pulsos) {
      c = pulsos;
      dis_recorrido_pulsos++;
      t = millis();
    } else {
      long now = millis();
      if (now - t > 300) {
        stado = 1;
      }
    }
  } while (stado == 0);
}
void leerSensores() {
  if (digitalRead(sensor1_in) == HIGH || digitalRead(sensor2_in) == HIGH) {
    t = millis();
    if (!sen_isDisable && state_puerta != ABIERTO) {
      if (state_direccion == CERRANDO) {
        Serial.print("Sensor activado: ");
        Serial.println(digitalRead(sensor1_in) == HIGH ? "1" : "2");
        sen_isActivado = true;
        sen_isDisable = true;
      }
    }
  }
}
void loop() {
  long now = millis();
  //if (Sta_calibrado)
  //{
  leerSensores();
  /*if (!Sta_avance) {
    Sta_avance = true;
    buscarPosFinal(true, abriendo,60);
    }*/

  /*cerrar puerta pasando un tiempo si boton abrierto no esta pulsado */
  if (state_puerta == ABIERTO && !pulAbrir_isEnable) {
    if (now - t > (tEspera * 1000)) {
      t = now;
      setDireccionGiro(CERRANDO);
      Serial.println("Cerrar a velocidad media");
      procOpenClose(dis_total_pulsos, vel_media);
      //buscarPosFinal(true, cerrando, 50);
    }
  }
  /* Abrir puerta cuando se activa el sensor de presencia */
  if (sen_isActivado) {
    if (state_puerta == IS_WORKING && state_direccion == CERRANDO) {
      if (dis_recorrido_pulsos > 0) {
        setDireccionGiro(ABRIENDO);
        //state_direccion = ABRIENDO;
        Serial.print("Retroceder: ");
        Serial.println(dis_recorrido_pulsos);
        if (dis_recorrido_pulsos > dis_total_pulsos / 2) { //Si puerta esta cerrando mas de la mitad de la distancia total
          Serial.println("Abrir a velocidad media");
          procOpenClose(dis_recorrido_pulsos, vel_media);
          // delay(1000);
          // buscarPosFinal(true, abriendo, 90); //60 MODIFICADO ACA
        } else {
          if (dis_recorrido_pulsos > 30) { // Si puerta empieza a cerrarse pulso es mayor a 30
            Serial.println("Abrir a velocidad 100");
            procOpenClose(dis_recorrido_pulsos, 100);
          } else { // Pulso no alcanzo 30 retroceder a velocidad minima hasta posicion final
            Serial.println("Abrir a velocidad minima");
            int d = dis_recorrido_pulsos;
            dis_recorrido_pulsos = 0;
            sen_isActivado = false;
            pulsos = 0;
            state_puerta = IS_WORKING;
            //setDireccionGiro(state_direccion);
            avanceMotor(90, 90, d);
            procCompletado();
          }

          //delay(1000);
          //buscarPosFinal(true, abriendo, 90);//60 MODIFICADO ACA
        }
      } else {
        sen_isActivado = false;
        state_puerta = ABIERTO;
        sen_isDisable = false;
        //setDireccionGiro(ABRIENDO);
        /* Buscar posicion final */
        buscarPosFinal(true, ABRIENDO, 90);  //60 MODIFICADO ACA
      }
    }
    if (state_direccion == CERRANDO && state_puerta == CERRADO) { // Cuando la puerta estado cerrado 
      setDireccionGiro(ABRIENDO);
      Serial.println("Abrir a Velocidad maxima");
      procOpenClose(dis_total_pulsos, VEL_MAX);
      //delay(1000);
      //buscarPosFinal(true, abriendo, 90);//60 MODIFICADO ACA
    }
  }
}

void procOpenClose(int distancia, byte v) {
  int D_aceleracion = 0;
  sen_isActivado = false;
  dis_recorrido_pulsos = 0;
  pulsos = 0;
  state_puerta = IS_WORKING;

  tiempo = millis();


  Serial.print("Proceso ");
  Serial.print(state_direccion == ABRIENDO ? " ABRIENDO" : " CERRANDO");
  Serial.println(" iniciando...");



  /* aranque - acelerando */
  Serial.println("Arranque acelerando desde velocidad minima");
  if (distancia == dis_total_pulsos) {  // Si es abrir o cerrar completo
    if (state_direccion == ABRIENDO) {
      avanceMotor(vel_minima, v, 29);
    } else {
      avanceMotor(vel_minima, v, 43);
    }

  } else {                             // si es abrir desde la mitad
    D_aceleracion = distancia * 0.15;  // Arranque aceleracion hasta 15% distancia total
    avanceMotor(vel_minima, v, D_aceleracion);
  }
  /*if (!Sta_avance) {
    parar(NOFrenar);
    delay(300);
    return;
    } */
  if (sen_isActivado) {
    detenerMotor(SI);  // Aplica frenado
    return;            // Si sensor esta activo sale del proceso y pasa a bucle loop
  }

  /* Velocidad normal */
  Serial.println("Mantener Velocidad");
  if (distancia == dis_total_pulsos) {
    if (state_direccion == ABRIENDO) {
      avanceMotor(v, v, 28);
    } else {
      avanceMotor(v, v, 14);
    }
  } else {
    D_aceleracion = distancia * 0.05; //5% de la distancia total
    avanceMotor(v, v, D_aceleracion);
  }
  // parada - desacelerando
  /*if (!Sta_avance) {
    parar(NOFrenar);
    delay(300);
    return;
    }*/
  if (sen_isActivado) {
    detenerMotor(SI);
    return;
  }

  Serial.println("Velocidad maxima desacelarando...");
  if (distancia == dis_total_pulsos) {
    if (state_direccion == ABRIENDO) {
      avanceMotor(v, VEL_FRENADO, 222);
      // return; -------------
    } else {
      //avanceMotor(v, 10, 224);
      avanceMotor(v, VEL_FRENADO, 222);
      /* Dar un empuje extra para abrir bien la puerta */
      girarMotor(30);
      delay(100);
      girarMotor(0);
    }

  } else { //Cuando la puerta no estaba cerrado
    D_aceleracion = distancia * 0.8;  //80% distancia aceleracion hasta parar
    avanceMotor(v, VEL_FRENADO, D_aceleracion);
  }
  if (sen_isActivado) {
    detenerMotor(SI);
    return;
  }
  /*
    if (!Sta_avance) {
    parar(NOFrenar);
    delay(300);
    return;
    }*/

  procCompletado();
}
void procCompletado() {
  Serial.println("");
  if (state_direccion == ABRIENDO) {
    state_puerta = ABIERTO;  // Finaliza proceso puerta abierta
  } else {
    state_puerta = CERRADO;  //Finaliza proceso puerta cerrada
  }
  t = millis();  // Resetea tiempo espera
  detenerMotor(NO);
  sen_isDisable = false;  // Activa Sensor para detectar presencia
  Serial.println("Proceso Completado");
  Serial.println(" ");
  detectarMovimiento();
}

void avanceMotor(float vel_inicial, float vel_final, float distancia) {
  int c = 0;
  int pul = 0;

  long now;
  isAvance = true;
  float incremento = (vel_final - vel_inicial) / distancia; // incremento para acelelar
  float vel = vel_inicial;
  t = millis();
  do {
    leerSensores();
    //leerSerial();

    now = millis();


    if (sen_isActivado) {
      break;  // Si sensor de prensencia se activa salta el bucle while
    } else {
      velActual = int(vel);
      girarMotor(velActual);
      // delay(5);
    }
    if (c != pulsos)  // Cuando el encoder lee un nuevo movimiento
    {
      if (vel_inicial != vel_final) {
        vel = vel + (incremento);
      }
      c = pulsos;
      dis_recorrido_pulsos++;
      t = now;

      Serial.print(dis_recorrido_pulsos);
      Serial.print("    v:");
      Serial.print(vel);
      Serial.print("    ");
      Serial.print(dis_recorrido_pulsos * 0.5);
      Serial.print("cm    ");
      Serial.print((millis() - tiempo) / 1000);
      Serial.println("s ");
    } else {
      if (dis_recorrido_pulsos > 60) {
        if (now - t > 1500) {
          sen_isActivado = true;
          break;
        }
      }
    }
  } while (pulsos < distancia);
  pulsos = 0;
}
void detectarMovimiento() {
  byte estado = ABIERTO;
  int pul = 0;
  long now;
  t = millis();
  do {
    now = millis();
    if (now - t > 200) {
      if (pulsos == pul) {
        Serial.print("Pulso encoder: ");
        Serial.print(pulsos);
        Serial.print(" Pul x: ");
        Serial.println(pul);
        estado = CERRADO;
      }
      t = now;
      pul = pulsos;
    }
  } while (estado == ABIERTO);
}