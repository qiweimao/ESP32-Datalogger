#ifndef LORA_USER_SETTINGS_H
#define LORA_USER_SETTINGS_H

#include "lora_init.h"

// Define user-specific message types
enum UserMessageType {
    TIME_SYNC = USER_DEFINED_START,
    USER_TYPE_2,
    USER_TYPE_3
    // Add more user-defined types here
};

#endif // USER_MESSAGE_TYPES_H
