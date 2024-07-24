#ifndef LORA_USER_SETTINGS_H
#define LORA_USER_SETTINGS_H

#include "lora_init.h"

// Define user-specific message types
enum UserMessageType {
    TIME_SYNC = USER_DEFINED_START,
    GET_CONFIG,
    DATA_CONFIG,
    SYS_CONFIG,
};

//Define the pins used by the transceiver module
#define LORA_RST 27
#define DIO0 4
#define LORA_SCK 14
#define LORA_MISO 12
#define LORA_MOSI 13
#define LORA_SS 15

#endif // USER_MESSAGE_TYPES_H
