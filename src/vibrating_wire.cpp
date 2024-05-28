#include "utils.h"
#include "vibrating_wire.h"
extern HardwareSerial VM; // UART port 1 on ESP32

const int MAX_COMMANDSIZE = 6;

unsigned int crc16(unsigned char *dat, unsigned int len)
{
    unsigned int crc = 0xffff;
    unsigned char i;
    while (len != 0)
    {
        crc ^= *dat;
        for (i = 0; i < 8; i++)
        {
            if ((crc & 0x0001) == 0)
                crc = crc >> 1;
            else
            {
                crc = crc >> 1;
                crc ^= 0xa001;
            }
        }
        len -= 1;
        dat++;
    }
    return crc;
}

void parseCommand(const char* command) {
    // Variables to store parsed values
    char commandName[6];
    uint8_t address;
    uint8_t functionCode;
    uint8_t startAddressHigh;
    uint8_t startAddressLow;
    uint8_t numRegistersHigh;
    uint8_t numRegistersLow;
    uint8_t crcHigh;
    uint8_t crcLow;

    uint8_t hexArray[MAX_COMMANDSIZE] = {};

// MODBUS 0x01 0x03 0x00 0x00 0x00 0x0A
// MODBUS 0x01 0x03 0x00 0x27 0x00 0x01 GET Sensor Resistance 
  if (strncmp(command, "MODBUS", 6) == 0) {
    // Use sscanf to parse the command string
    int result = sscanf(command, "%s 0x%2hhx 0x%2hhx 0x%2hhx 0x%2hhx 0x%2hhx 0x%2hhx",
                        &commandName, 
                        &hexArray[0], &hexArray[1], &hexArray[2], &hexArray[3], 
                        &hexArray[4], &hexArray[5]);

    // Check if all values were successfully parsed
    if (result - 1 == MAX_COMMANDSIZE) {
      unsigned int crc = crc16(hexArray, sizeof(hexArray));
      uint8_t crcHighByte = (uint8_t)(crc >> 8);
      uint8_t crcLowByte = (uint8_t)(crc & 0xFF);

      VM.write(hexArray, sizeof(hexArray));
      VM.write(crcLowByte);
      VM.write(crcHighByte);

      delay(1000);
      Serial.printf("VM501:");
      while (VM.available() > 0) {
        uint8_t incomingByte = VM.read();
        Serial.printf("0x%02x ", incomingByte);
      }
      Serial.printf("\n");
    }
    else {
        Serial.println("Invalid MODBUS command");
    }
  }
  else if (strncmp(command, "$", 1) == 0){

    VM.write(command);
    VM.write("\n");

    delay(3000);

    if (VM.available() > 0) {
      String receivedChar = VM.readString();
      Serial.println(receivedChar);
    }
    else{
      Serial.println("No reponse from VM501.");
    }
  }
  else{
    Serial.println("Invalid command");
  }
}

void vm501_init() {
  VM.begin(9600, SERIAL_8N1, 16, 17); // Initialize UART port 1 with GPIO16 as RX and GPIO17 as TX
}

String readVM(){
    const char * command = "$MSFT=3\n";
    VM.write(command);
    delay(500);
    if (VM.available() > 0) {
        String receivedChar = VM.readString();
        return receivedChar;
    }
    else{
        return "NaN";
    }
}

void sendCommandVM501(void *parameter) {
    while (true) {
        // Check if data is available on the serial port
        if (Serial.available() > 0) {
            // Read the input command from the serial port
            String input = Serial.readStringUntil('\n');
            // Convert String to C-string
            char command[input.length() + 1];
            input.toCharArray(command, sizeof(command));

            // Parse the command
            parseCommand(command);
        }
        // Delay for a short period to avoid busy-waiting
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}




