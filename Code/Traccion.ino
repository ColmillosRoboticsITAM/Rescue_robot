// ================= defines =================
const int pinTR = 32;
const int pinTL = 33;

// configrar pwm
const int freq = 50;
const int resolution = 16;

// ================= setup =================
void setup() {
  Serial.begin(115200);

  ledcAttach(pinTR, freq, resolution);
  ledcAttach(pinTL, freq, resolution);

  Serial.println("TRACCION READY");
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

// ================= loop =================
void loop() {

  if (Serial.available()) {

    String line = Serial.readStringUntil('\n');
    line.trim();

    float values[2];
    int index = 0;

    char buffer[50];
    line.toCharArray(buffer, 50);

    char *token = strtok(buffer, ",");

    while (token != NULL && index < 2) {
      values[index++] = atof(token);
      token = strtok(NULL, ",");
    }

    if (index == 2) {

      float linear  = constrain(values[0], -1.0, 1.0);
      float angular = constrain(values[1], -1.0, 1.0);

      moveRobot(linear, angular);

      Serial.print("OK: ");
      Serial.println(line);
    }
    else {
      Serial.println("ERROR: usa formato v,w");
    }
  }
}