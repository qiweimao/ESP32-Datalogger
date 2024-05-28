#ifndef LORA_INIT_H
#define LORA_INIT_H

#include "utils.h"

typedef struct vm {
  float freq;
  float temp;
} vm;

typedef struct struct_message {
  uint8_t msgType;
  uint8_t mac[6];
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;


typedef struct vm_message {
  uint8_t msgType;
  uint8_t mac[6];
  const char* time;
  vm vm_data[3];
} vm_message;

typedef struct adc {
  uint8_t msgType;
  uint8_t mac[6];
  const char* time;
  float adc[16];
} adc;

typedef struct struct_pairing {       // new structure for pairing
  uint8_t msgType;
  uint8_t mac[6];
  uint32_t pairingKey;
} struct_pairing;

// Structure to hold task parameters
struct TaskParams {
  unsigned long interval;
};

enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};

enum MessageType {PAIRING, DATA,};

extern uint8_t mac_buffer[6];

void LoRa_rxMode();
void LoRa_txMode();
void LoRa_sendMessage(String message);
void onReceive(int packetSize);
void onTxDone();
boolean runEvery(unsigned long interval);
void loopFunction(void *parameter);
void handleReceivedData(void *parameter);
void taskReceive(void *parameter);

#endif