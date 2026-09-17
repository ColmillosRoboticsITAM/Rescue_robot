#include <ESP32-TWAI-CAN.hpp>

#define SPARK_DEVICE_ID 5
#define HEARTBEAT_ID    0x2052C80
#define DUTY_CYCLE_ID   (0x2050080 | SPARK_DEVICE_ID)  // 0x2050085

#define CAN_TX_PIN 21
#define CAN_RX_PIN 22

void floatToBytes(float val, uint8_t* buf) {
  memcpy(buf, &val, 4);
}

void sendHeartbeat() {
  CanFrame frame;
  frame.identifier       = HEARTBEAT_ID;
  frame.extd             = 1;
  frame.data_length_code = 8;
  memset(frame.data, 0x00, 8);
  frame.data[0]          = 0x20;  // bit 5 = Device ID 5
  ESP32Can.writeFrame(frame);
}

void sendDutyCycle(float speed) {
  if (speed >  1.0f) speed =  1.0f;
  if (speed < -1.0f) speed = -1.0f;
  CanFrame frame;
  frame.identifier       = DUTY_CYCLE_ID;
  frame.extd             = 1;
  frame.data_length_code = 8;
  memset(frame.data, 0x00, 8);
  floatToBytes(speed, frame.data);
  ESP32Can.writeFrame(frame);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Forzar CTX en alto antes de iniciar TWAI
  pinMode(CAN_TX_PIN, OUTPUT);
  digitalWrite(CAN_TX_PIN, HIGH);
  delay(100);

  ESP32Can.setPins(CAN_TX_PIN, CAN_RX_PIN);
  ESP32Can.setSpeed(ESP32Can.convertSpeed(1000));

  if (!ESP32Can.begin()) {
    Serial.println("Error iniciando CAN");
    while (1);
  }

  Serial.println("CAN listo — ESP32 + SN65HVD230");
  Serial.println("Comandos: f=adelante 30%, b=reversa 30%, s=stop");
}

unsigned long lastHB  = 0;
unsigned long lastCmd = 0;
unsigned long lastLog = 0;
float speed           = 0.0f;
float lastApplied     = 0.0f;

void loop() {
  unsigned long now = millis();

  if (Serial.available()) {
    char c = Serial.read();
    if      (c == 'f') { speed =  0.1f; Serial.println(">> Adelante 30%"); }
    else if (c == 'b') { speed = -0.1f; Serial.println(">> Reversa 30%");  }
    else if (c == 's') { speed =  0.0f; Serial.println(">> Stop");         }
  }

  if (now - lastHB >= 20) {
    sendHeartbeat();
    lastHB = now;
  }

  if (now - lastCmd >= 20) {
    sendDutyCycle(speed);
    lastCmd = now;
  }

    // Leer telemetria
  CanFrame rx;
  if (ESP32Can.readFrame(rx, 0) && rx.extd) {
    
    if (rx.identifier == (0x205B800 | SPARK_DEVICE_ID)) {
      int16_t raw = (int16_t)(rx.data[0] | (rx.data[1] << 8));
      lastApplied = raw / 32767.0f;
    }

    if (rx.identifier == (0x205B880 | SPARK_DEVICE_ID)) {
      float velocity, position;
      memcpy(&velocity, &rx.data[0], 4);
      memcpy(&position, &rx.data[4], 4);
      Serial.print("Vel: "); Serial.print(velocity, 3);
      Serial.print("  Pos: "); Serial.println(position, 3);
    }
  }

  if (now - lastLog >= 100) {
    Serial.print("Applied: "); Serial.println(lastApplied, 3);
    lastLog = now;
  }
}