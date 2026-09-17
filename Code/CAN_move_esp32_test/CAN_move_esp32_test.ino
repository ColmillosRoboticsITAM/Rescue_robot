#include <ESP32-TWAI-CAN.hpp>

#define CAN_TX_PIN 21
#define CAN_RX_PIN 22

#define HEARTBEAT_ID 0x2052C80UL

// ── Clase SparkMax ──────────────────────────────────────
class SparkMax {
public:
  uint8_t deviceId;
  float   appliedOutput;
  float   velocity;
  float   position;

  SparkMax(uint8_t id) : deviceId(id), appliedOutput(0), velocity(0), position(0) {}

  void sendDutyCycle(float speed) {
    speed = constrain(speed, -1.0f, 1.0f);
    uint32_t id = 0x2050080UL | deviceId;
    CanFrame f;
    f.identifier       = id;
    f.extd             = 1;
    f.data_length_code = 8;
    memset(f.data, 0x00, 8);
    memcpy(f.data, &speed, 4);
    ESP32Can.writeFrame(f);
  }

  // Procesa un frame recibido — retorna true si era para este motor
  bool processFrame(uint32_t frameId, uint8_t* data) {
    uint32_t base  = frameId & ~0x3FUL;
    uint8_t  devId = frameId & 0x3F;

    if (devId != deviceId) return false;

    if (base == 0x205B800UL) {
      // STATUS_0 — Applied Output
      int16_t raw = (int16_t)(data[0] | (data[1] << 8));
      appliedOutput = raw / 32767.0f;
      return true;
    }

    if (base == 0x205B880UL) {
      // STATUS_2 — Velocity y Position
      memcpy(&velocity, &data[0], 4);
      memcpy(&position, &data[4], 4);
      return true;
    }

    return false;
  }

  void print() {
    Serial.print("SPARK["); Serial.print(deviceId); Serial.print("] ");
    Serial.print("Applied:"); Serial.print(appliedOutput, 3);
    Serial.print(" Vel:"); Serial.print(velocity, 1);
    Serial.print(" Pos:"); Serial.println(position, 1);
  }
};

// ── Instancias ──────────────────────────────────────────
SparkMax motor1(5);  // Device ID 5
SparkMax motor2(4);  // Device ID 4

// ── Heartbeat broadcast ─────────────────────────────────
// Un solo heartbeat habilita ambos motores
void sendHeartbeat() {
  CanFrame f;
  f.identifier       = HEARTBEAT_ID;
  f.extd             = 1;
  f.data_length_code = 8;
  memset(f.data, 0x00, 8);
  // bit 4 = DevID 4, bit 5 = DevID 5
  f.data[0] = (1 << 4) | (1 << 5);  // 0x30
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

  Serial.println("CAN listo — 2x SPARK MAX");
  Serial.println("Comandos:");
  Serial.println("  f = ambos adelante 10%");
  Serial.println("  b = ambos reversa 10%");
  Serial.println("  s = stop ambos");
  Serial.println("  1 = solo motor1 adelante");
  Serial.println("  2 = solo motor2 adelante");
}

// ── Loop ────────────────────────────────────────────────
unsigned long lastHB   = 0;
unsigned long lastCmd  = 10;
unsigned long lastLog  = 0;

float speed1 = 0.0f;
float speed2 = 0.0f;

void loop() {
  unsigned long now = millis();

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'f') {
      speed1 =  0.05f; speed2 =  0.05f;
      Serial.println(">> Ambos adelante");
    } else if (c == 'b') {
      speed1 = -0.05f; speed2 = -0.05f;
      Serial.println(">> Ambos reversa");
    } else if (c == 's') {
      speed1 =  0.0f; speed2 =  0.0f;
      Serial.println(">> Stop");
    } else if (c == '1') {
      speed1 =  0.05f; speed2 =  0.0f;
      Serial.println(">> Solo motor1");
    } else if (c == '2') {
      speed1 =  0.0f; speed2 =  0.05f;
      Serial.println(">> Solo motor2");
    }
  }

  // Heartbeat — habilita ambos con un solo frame
  if (now - lastHB >= 20) {
    sendHeartbeat();
    lastHB = now;
  }

  // Comandos — alternar entre motor1 y motor2
  if (now - lastCmd >= 10) {
    motor1.sendDutyCycle(speed1);
    motor2.sendDutyCycle(speed2);
    lastCmd = now;
  }

  // Telemetría
  CanFrame rx;
  if (ESP32Can.readFrame(rx, 0) && rx.extd) {
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