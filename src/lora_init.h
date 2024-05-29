#ifndef LORA_INIT_H
#define LORA_INIT_H

#include "utils.h"
#include <LoRa.h>

#define LORA_SLAVE 0
#define LORA_GATEWAY 1

#define CHUNK_SIZE 200  // Size of each chunk
#define ACK_TIMEOUT 2000  // Timeout for ACK in milliseconds

typedef struct vm {
  float freq;
  float temp;
} vm;

typedef struct file_meta_message {
  uint8_t msgType;
  uint8_t mac[6];
  char filename[40];
  uint32_t filesize;
} file_meta_message;

typedef struct file_body_message {
  uint8_t msgType;
  uint8_t mac[6];
  uint8_t data[CHUNK_SIZE];
  uint8_t len;
} file_body_message;

typedef struct file_end_message {
  uint8_t msgType;
  uint8_t mac[6];
} file_end_message;

typedef struct struct_message {
  uint8_t msgType;
  uint8_t mac[6];
  float temp;
  float hum;
  unsigned int readingId;
} struct_message;

// 3 channel VM message
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
  uint8_t mac[6]; // identity for device
  uint32_t pairingKey; // key for network
  char deviceName[10];
} struct_pairing;

struct TaskParams {// Structure to hold task parameters
  unsigned long interval;
};

enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};
enum MessageType {PAIRING, DATA, DATA_VM, DATA_ADC, DATA_I2C, DATA_SAA, FILE_META, FILE_BODY, FILE_END, ACK};

extern uint8_t mac_buffer[6];
extern uint8_t MAC_ADDRESS_STA[6];
extern int ack_count;

void LoRa_rxMode();
void LoRa_txMode();
void LoRa_sendMessage(String message);
void onReceive(int packetSize);
void onTxDone();
boolean runEvery(unsigned long interval);
void loopFunction(void *parameter);
void handleReceivedData(void *parameter);
void taskReceive(void *parameter);
void sendLoraMessage(uint8_t* data, size_t size);

#endif