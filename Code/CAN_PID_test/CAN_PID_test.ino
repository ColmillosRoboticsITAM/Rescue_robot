#include <ESP32-TWAI-CAN.hpp>

#define CAN_TX_PIN      21
#define CAN_RX_PIN      22
#define HEARTBEAT_ID    0x2052C80UL
#define SPARK_DEVICE_ID 5

#define POSITION_SETPOINT_ID (0x2050100UL | SPARK_DEVICE_ID)  // 0x2050105

void sendHeartbeat() {
  CanFrame f;
  f.identifier       = HEARTBEAT_ID;
  f.extd             = 1;
  f.data_length_code = 8;
  memset(f.data, 0x00, 8);
  f.data[0] = (1 << SPARK_DEVICE_ID);
  ESP32Can.writeFrame(f);
}

void sendPosition(float rotations) {
  CanFrame f;
  f.identifier       = POSITION_SETPOINT_ID;
  f.extd             = 1;
  f.data_length_code = 8;
  memset(f.data, 0x00, 8);
  memcpy(f.data, &rotations, 4);
  ESP32Can.writeFrame(f);
}

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

  Serial.println("POSITION READY");
  Serial.println("Manda posicion en rotaciones (ej: 0, 20, -5)");
}

unsigned long lastHB  = 0;
unsigned long lastCmd = 10;
unsigned long lastLog = 0;
float targetPosition  = 0.0f;
float currentPosition = 0.0f;

void loop() {
  unsigned long now = millis();

  if (now - lastHB >= 20) {
    sendHeartbeat();
    lastHB = now;
  }

  if (now - lastCmd >= 20) {
    float error = targetPosition - currentPosition;
    if (abs(error) <= 0.25f) {
      sendPosition(currentPosition);
    } else {
      sendPosition(targetPosition);
    }
    lastCmd = now;
  }

  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    targetPosition = line.toFloat();
    Serial.print(">> Target: "); Serial.println(targetPosition, 3);
  }

  CanFrame rx;
  if (ESP32Can.readFrame(rx, 0) && rx.extd) {
    if ((rx.identifier & ~0x3FUL) == 0x205B880UL &&
        (rx.identifier & 0x3F) == SPARK_DEVICE_ID) {
      memcpy(&currentPosition, &rx.data[4], 4);
    }
  }

  if (now - lastLog >= 150) {
    Serial.print("Target: "); Serial.print(targetPosition, 2);
    Serial.print("  Pos: ");  Serial.print(currentPosition, 2);
    Serial.print("  Error: "); Serial.println(targetPosition - currentPosition, 2);
    lastLog = now;
  }
}