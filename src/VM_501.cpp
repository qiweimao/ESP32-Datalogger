#include "utils.h"
#include "VM_501.h"
extern HardwareSerial VM; // UART port 1 on ESP32

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