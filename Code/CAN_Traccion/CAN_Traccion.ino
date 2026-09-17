#include <ESP32-TWAI-CAN.hpp>

#define CAN_TX_PIN   21
#define CAN_RX_PIN   22
#define HEARTBEAT_ID 0x2052C80UL

// ── Clase SparkMax ──────────────────────────────────────
class SparkMax {
public:
  uint8_t deviceId;
  float   appliedOutput;

  SparkMax(uint8_t id) : deviceId(id), appliedOutput(0) {}

  void sendDutyCycle(float speed) {
    speed = constrain(speed, -1.0f, 1.0f);
    CanFrame f;
    f.identifier       = 0x2050080UL | deviceId;
    f.extd             = 1;
    f.data_length_code = 8;
    memset(f.data, 0x00, 8);
    memcpy(f.data, &speed, 4);
    ESP32Can.writeFrame(f);
  }

  bool processFrame(uint32_t frameId, uint8_t* data) {
    if ((frameId & 0x3F) != deviceId) return false;
    if ((frameId & ~0x3FUL) == 0x205B800UL) {
      int16_t raw = (int16_t)(data[0] | (data[1] << 8));
      appliedOutput = raw / 32767.0f;
      return true;
    }
    return false;
  }
};

// ── Instancias ──────────────────────────────────────────
SparkMax motorL(4);  // Tracción izquierda — Device ID 4
SparkMax motorR(5);  // Tracción derecha   — Device ID 5

// ── Heartbeat ───────────────────────────────────────────
void sendHeartbeat() {
  CanFrame f;
  f.identifier       = HEARTBEAT_ID;
  f.extd             = 1;
  f.data_length_code = 8;
  memset(f.data, 0x00, 8);
  f.data[0] = (1 << 4) | (1 << 5);  // 0x30: DevID 4 y 5
  ESP32Can.writeFrame(f);
}

// ── Tracción ────────────────────────────────────────────
void moveRobot(float v, float w) {
  float left  = v - w;
  float right = v + w;

  float maxVal = max(abs(left), abs(right));
  if (maxVal > 1.0f) {
    left  /= maxVal;
    right /= maxVal;
  }

  if (abs(left)  < 0.05f) left  = 0;
  if (abs(right) < 0.05f) right = 0;

  // Invertir izquierdo (mismo comportamiento que el código PWM original)
  motorL.sendDutyCycle(-left);
  motorR.sendDutyCycle(right);
}

// ── Setup ───────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(CAN_TX_PIN, OUTPUT);
  digitalWrite(CAN_TX_PIN, HIGH);
  delay(100);

  ESP32Can.setPins(CAN_TX_PIN, CAN_RX_PIN);
  ESP32Can.setSpeed(ESP32Can.convertSpeed(1000));

  if (!ESP32Can.begin()) {
    Serial.println("Error CAN");
    while (1);
  }

  Serial.println("TRACCION READY");
}

// ── Loop ────────────────────────────────────────────────
unsigned long lastHB  = 0;
unsigned long lastLog = 0;

void loop() {
  unsigned long now = millis();

  // Heartbeat cada 20ms
  if (now - lastHB >= 20) {
    sendHeartbeat();
    lastHB = now;
  }

  // Leer comando serial
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    float values[2];
    int index = 0;
    char buffer[50];
    line.toCharArray(buffer, 50);
    char* token = strtok(buffer, ",");
    while (token != NULL && index < 2) {
      values[index++] = atof(token);
      token = strtok(NULL, ",");
    }

    if (index == 2) {
      float linear  = constrain(values[0], -1.0f, 1.0f);
      float angular = constrain(values[1], -1.0f, 1.0f);
      moveRobot(linear, angular);
      Serial.print("OK: "); Serial.println(line);
    } else {
      Serial.println("ERROR: usa formato v,w");
    }
  }

  // Telemetría
  CanFrame rx;
  if (ESP32Can.readFrame(rx, 0) && rx.extd) {
    motorL.processFrame(rx.identifier, rx.data);
    motorR.processFrame(rx.identifier, rx.data);
  }

  // Print cada 500ms
  if (now - lastLog >= 500) {
    Serial.print("L:"); Serial.print(motorL.appliedOutput, 3);
    Serial.print(" R:"); Serial.println(motorR.appliedOutput, 3);
    lastLog = now;
  }
}