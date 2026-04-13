// ================= defines =================
const int pinFlp1 = 13;
const int pinFlp2 = 12;
const int pinFlp3 = 14;
const int pinFlp4 = 27;
const int pinTR   = 32;
const int pinTL   = 33;

// configrar pwm
const int freq = 50;
const int resolution = 16;

// ================= setup =================
void setup() {
  Serial.begin(115200);

  ledcAttach(pinFlp1, freq, resolution);
  ledcAttach(pinFlp2, freq, resolution);
  ledcAttach(pinFlp3, freq, resolution);
  ledcAttach(pinFlp4, freq, resolution);
  ledcAttach(pinTR,   freq, resolution);
  ledcAttach(pinTL,   freq, resolution);

  Serial.println("READY");
}

// ================= micro_s a duty =================
uint32_t usToDuty(int us) {
  float duty = (float)us / 20000.0;
  return (uint32_t)(duty * ((1 << resolution) - 1));
}

// ================= pwm =================
void setPWMus(int pin, int us) {
  us = constrain(us, 1000, 2000);
  ledcWrite(pin, usToDuty(us));
}

// ================= traccion =================
void moveRobot(float v, float w) {

  float left  = v - w;
  float right = v + w;

  float maxVal = max(abs(left), abs(right));
  if (maxVal > 1.0) {
    left  /= maxVal;
    right /= maxVal;
  }

  if (abs(left) < 0.05) left = 0;
  if (abs(right) < 0.05) right = 0;

  int pwmLeft  = 1500 + left * 400;
  int pwmRight = 1500 + right * 400;

  pwmLeft = 1500 - (pwmLeft - 1500);

  setPWMus(pinTL, pwmLeft);
  setPWMus(pinTR, pwmRight);
}

// ================= flippers =================
void moveFlippers(float f1, float f2, float f3, float f4) {

  // deadband
  if (abs(f1) < 0.05) f1 = 0;
  if (abs(f2) < 0.05) f2 = 0;
  if (abs(f3) < 0.05) f3 = 0;
  if (abs(f4) < 0.05) f4 = 0;

  int pwm1 = 1500 + f1 * 400;
  int pwm2 = 1500 - (f2 * 400);
  int pwm3 = 1500 + f3 * 400;
  int pwm4 = 1500 - (f4 * 400);

  setPWMus(pinFlp1, pwm1);
  setPWMus(pinFlp2, pwm2);
  setPWMus(pinFlp3, pwm3);
  setPWMus(pinFlp4, pwm4);
}

// ================= loop =================
void loop() {

  if (Serial.available()) {

    String line = Serial.readStringUntil('\n');
    line.trim();

    float values[6];
    int index = 0;

    char buffer[100];
    line.toCharArray(buffer, 100);

    char *token = strtok(buffer, ",");

    while (token != NULL && index < 6) {
      values[index++] = atof(token);
      token = strtok(NULL, ",");
    }

    if (index == 6) {

      // flippers
      float f1 = constrain(values[0], -1.0, 1.0);
      float f2 = constrain(values[1], -1.0, 1.0);
      float f3 = constrain(values[2], -1.0, 1.0);
      float f4 = constrain(values[3], -1.0, 1.0);

      moveFlippers(f1, f2, f3, f4);

      // tracción
      float linear  = constrain(values[4], -1.0, 1.0);
      float angular = constrain(values[5], -1.0, 1.0);

      moveRobot(linear, angular);

      Serial.print("OK: ");
      Serial.println(line);
    }
    else {
      Serial.println("ERROR");
    }
  }
}
/*
==================== pruebas ====================

1) ADELANTE
Comando:
0,0,0,0,1,0
Robot va recto hacia adelante

2) ATRÁS
Comando:
0,0,0,0,-1,0
Robot va recto hacia atrás

3) GIRO DERECHA (en su eje)
Comando:
0,0,0,0,0,1
- Izquierdo adelante
- Derecho atrás
- Robot gira sobre su eje a la derecha

4) GIRO IZQUIERDA (en su eje)
Comando:
0,0,0,0,0,-1
- Izquierdo atrás
- Derecho adelante
- Robot gira sobre su eje a la izquierda

5) CURVA DERECHA (suave)
Comando:
0,0,0,0,0.5,0.5
- Avanza girando a la derecha
- Un lado más rápido que el otro

6) CURVA IZQUIERDA (suave)
Comando:
0,0,0,0,0.5,-0.5
- Avanza girando a la izquierda

7) STOP
Comando:
0,0,0,0,0,0
- Todo se detiene
*/