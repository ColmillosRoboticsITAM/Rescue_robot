#include <ESP32-TWAI-CAN.hpp>

#define CAN_TX_PIN   21
#define CAN_RX_PIN   22
#define HEARTBEAT_ID 0x2052C80UL

// ── Clase SparkMax ──────────────────────────────────────
class SparkMax {
public:
  uint8_t deviceId;
  float   currentPosition;
  float   targetPosition;

  SparkMax(uint8_t id) : deviceId(id), currentPosition(0), targetPosition(0) {}

  void sendPosition(float rotations) {
    CanFrame f;
    f.identifier       = 0x2050100UL | deviceId;
    f.extd             = 1;
    f.data_length_code = 8;
    memset(f.data, 0x00, 8);
    memcpy(f.data, &rotations, 4);
    ESP32Can.writeFrame(f);
  }

    void update() {
    sendPosition(targetPosition);  
    // siempre mandar target — el PID interno maneja el resto
  }

  bool processFrame(uint32_t frameId, uint8_t* data) {
    if ((frameId & 0x3F) != deviceId) return false;
    if ((frameId & ~0x3FUL) == 0x205B880UL) {
      memcpy(&currentPosition, &data[4], 4);
      return true;
    }
    return false;
  }

  void print() {
    Serial.print("SPARK["); Serial.print(deviceId); Serial.print("]");
    Serial.print(" Target:"); Serial.print(targetPosition, 2);
    Serial.print(" Pos:"); Serial.println(currentPosition, 2);
  }
};

// ── Instancias ──────────────────────────────────────────
SparkMax motor1(4);
SparkMax motor2(5);

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

  Serial.println("POSITION READY — 2x SPARK MAX");
  Serial.println("Formato: setpoint1,setpoint2  (ej: 10,20)");
}

// ── Loop ────────────────────────────────────────────────
unsigned long lastHB  = 0;
unsigned long lastCmd = 10;
unsigned long lastLog = 0;

void loop() {
  unsigned long now = millis();

  // Heartbeat cada 20ms
  if (now - lastHB >= 20) {
    sendHeartbeat();
    lastHB = now;
  }

  // Setpoint cada 20ms
    if (now - lastCmd >= 20) {
    motor1.update();
    delayMicroseconds(200);
    motor2.update();
    lastCmd = now;
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
      motor1.targetPosition = values[0];
      motor2.targetPosition = values[1];
      Serial.print(">> Target: "); Serial.print(values[0], 2);
      Serial.print(", "); Serial.println(values[1], 2);
    } else {
      Serial.println("ERROR: usa formato sp1,sp2");
    }
  }

    // Telemetría — leer hasta 10 frames por ciclo
  CanFrame rx;
  for (int i = 0; i < 10; i++) {
    if (!ESP32Can.readFrame(rx, 0)) break;
    if (!rx.extd) continue;
    motor1.processFrame(rx.identifier, rx.data);
    motor2.processFrame(rx.identifier, rx.data);
  }

  // Print cada 500ms
  if (now - lastLog >= 500) {
    motor1.print();
    motor2.print();
    lastLog = now;
  }
}