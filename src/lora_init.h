#ifndef LORA_INIT_H
#define LORA_INIT_H

#include "utils.h"

typedef struct struct_message {
  uint8_t msgType;
  uint8_t id;
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

typedef struct struct_pairing {       // new structure for pairing
    uint8_t msgType;
    uint8_t id;
} struct_pairing;

enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};

enum MessageType {PAIRING, DATA,};

void printMAC(const uint8_t mac_addr);

#endif