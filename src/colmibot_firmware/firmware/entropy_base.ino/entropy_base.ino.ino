/* Move_Robot
 Programa de prueba para el movimiento de un robot diferencial de entropia utilizando ESP32 y 6 Sparks
 para controlar traccion y posición de los flippers */

// ------------- Pines ----------------

// Pines de conexión de los motores
#define pinFlp1 13
#define pinFlp2 12
#define pinFlp3 14
#define pinFlp4 27
#define pinTR 32
#define pinTL 33

// configrar pwm
const int freq = 50;
const int resolution = 16;

String input = "";

// ------------- Setup ----------------

void setup() {
  // Configuración de pines como salida 
  ledcAttach(pinFlp1, freq, resolution);
  ledcAttach(pinFlp2, freq, resolution);
  ledcAttach(pinFlp3, freq, resolution);
  ledcAttach(pinFlp4, freq, resolution);
  ledcAttach(pinTR,   freq, resolution);
  ledcAttach(pinTL,   freq, resolution);

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
    moveFlippers(v1, v2, v3, v4);

  }
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

void moveRobot(float linealVel, float angularVel) {
  float WS = 0.55;        // Wheel_Separation (m) - 20 cm
  
  float max_vel = 1.0;     // velocidad máxima esperada (m/s) 0.52
  // 165 RPM = 17.28 rad/s
  // v = omega * wheel_radius = 17.28 * 0.03
  // v = 0.5283 m/s

  // Cinemática
  float v_left  = linealVel - (WS / 2.0) * angularVel;
  float v_right = linealVel + (WS / 2.0) * angularVel;

  float maxVal = max(abs(v_left), abs(v_right));
  if (maxVal > 1.0) {
    v_left  /= maxVal;
    v_right /= maxVal;
  }

  if (abs(v_left) < 0.05) v_left = 0;
  if (abs(v_right) < 0.05) v_right = 0;

  int pwmLeft  = 1500 + v_left * 400;
  int pwmRight = 1500 + v_right * 400;

  pwmLeft = 1500 - (pwmLeft - 1500);

  setPWMus(pinTL, pwmLeft);
  setPWMus(pinTR, pwmRight);
}

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



