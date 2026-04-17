/* Move_Robot
 Programa de prueba para el movimiento de un robot diferencial utilizando ESP32 y un Cytron de 2 canales
 el cual realiza en una rutina infinita con los 4 movimientos en el robot básico y detenido */

// ------------- Pines ----------------

// Pines de conexión de los motores
#define LMot_dir 32     // Dir1 Cytron
#define LMot_pwm 33      // PWM1 Cytron
#define RMot_dir 25      // Dir2 Cytron 
#define RMot_pwm 26      // PWM2 Cytron

String input = "";

// ------------- Setup ----------------

void setup() {
  // Configuración de pines como salida o entrada
  pinMode(LMot_dir, OUTPUT);
  pinMode(RMot_dir, OUTPUT);
  Serial.begin(115200);
}

// ------------- Function ----------------

void processData(String data) {

  float values[6] = {0};

  int i = 0;
  char *token = strtok((char*)data.c_str(), ",");

  while (token != NULL && i < 6) {
    values[i++] = atof(token);
    token = strtok(NULL, ",");
  }

  // Asegurar que llegaron 6 valores
  if (i == 6) {

    float v1 = values[0];
    float v2 = values[1];
    float v3 = values[2];
    float v4 = values[3];
    float v5 = values[4];
    float v6 = values[5];

    moveRobot(v5, v6);

  }
}


void moveRobot(float linealVel, float angularVel) {
  float WS = 0.2;        // Wheel_Separation (m) - 20 cm
  
  float max_vel = 0.5;     // velocidad máxima esperada (m/s) 0.52
  // 165 RPM = 17.28 rad/s
  // v = omega * wheel_radius = 17.28 * 0.03
  // v = 0.5283 m/s

  // Cinemática
  float v_left  = linealVel - (WS / 2.0) * angularVel;
  float v_right = linealVel + (WS / 2.0) * angularVel;

  // Normalización de la velocidad [-1, 1] - El escalamiento en el archivo joy_teleop.yaml cambia
  v_left  = v_left  / max_vel;
  v_right = v_right / max_vel;

  // Restringir velocidad
  v_left  = constrain(v_left,  -1.0, 1.0);            // Restringuiendo límites para rueda izquierda para no salturarla
  v_right = constrain(v_right, -1.0, 1.0);            // Restringuiendo límites para rueda derecha para no salturarla

  int pwm_left  = (int)(v_left  * 255.0);         // Se escala para tener el valor de PWM de la rueda izquierda
  int pwm_right = (int)(v_right * 255.0);         // Se escala para tener el valor de PWM de la rueda derecha
  

// Condiciones para establecer los 4 movimientos del robot
  if (pwm_left >= 0) {
    digitalWrite(LMot_dir, LOW);
  } else {
    digitalWrite(LMot_dir, HIGH);
  }

  if (pwm_right >= 0) {
    digitalWrite(RMot_dir, LOW);
  } else {
    digitalWrite(RMot_dir, HIGH);
  }

  // Aplicar señales PWM a los motores
  analogWrite(LMot_pwm, abs(pwm_left));
  analogWrite(RMot_pwm, abs(pwm_right));
}

// ------------- Loop ----------------

void loop() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {
      processData(input);
      input = "";
    } else {
      input += c;
    }
  }
}



