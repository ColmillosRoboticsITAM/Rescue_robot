/**
 * Can bus sniffer - William Guimont-Martin 2025-2026
 * Adaptado para Arduino Mega con pin CS en 53
 */
#include <SPI.h>
#include <mcp2515.h>

// CS pin 53 para Arduino Mega
MCP2515 mcp2515(53, 4000000ul);

struct can_frame canMsg;

void setup()
{
    Serial.begin(115200);
    while (!Serial);

    Serial.println("CAN SNIFFER - Arduino Mega");

    mcp2515.reset();
    mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ);
    mcp2515.setNormalMode();
}

void loop()
{
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK)
    {
        Serial.print("ID: 0x");
        Serial.print(canMsg.can_id, HEX);
        Serial.print("  [");
        Serial.print(canMsg.can_dlc);
        Serial.print("]  ");
        for (int i = 0; i < canMsg.can_dlc; i++) {
            if (canMsg.data[i] < 0x10) Serial.print("0");
            Serial.print(canMsg.data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
}