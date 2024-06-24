#ifndef LORA_INIT_H
#define LORA_INIT_H

#include "utils.h"
#include <LoRa.h>
#include "lora_peer.h"

#define LORA_SLAVE 0
#define LORA_GATEWAY 1
#define CHUNK_SIZE 200  // Size of each chunk
#define ACK_TIMEOUT 5000  // Timeout for ACK in milliseconds
#define MAX_DEVICE_NAME_LEN 10  
#define MAX_FILENAME_LEN 20

/* File Transfer for Large Data  */
typedef struct file_body_message {
  uint8_t msgType;
  uint8_t mac[MAC_ADDR_LENGTH];
  char filename[MAX_FILENAME_LEN];
  uint32_t filesize;
  uint8_t len;
  uint8_t data[CHUNK_SIZE];
} file_body_message;

typedef struct signal {
  uint8_t msgType;
  uint8_t mac[MAC_ADDR_LENGTH];
} signal_message;

typedef struct time_sync_message {
  uint8_t msgType;
  uint32_t pairingKey; // key for network
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
} time_sync_message;

typedef struct struct_pairing { // this is a broadcast message
  uint8_t msgType;
  uint8_t mac_origin[MAC_ADDR_LENGTH]; // identity for device
  uint8_t mac_master[MAC_ADDR_LENGTH]; // identity for master
  uint32_t pairingKey; // key for network
  char deviceName[MAX_DEVICE_NAME_LEN];
} struct_pairing;

enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};
enum MessageType {PAIRING, DATA_VM, DATA_ADC, DATA_I2C, DATA_SAA, FILE_META, \
                  FILE_BODY, FILE_SYNC, ACK, REJ, TIMEOUT, TIME_SYNC, 
                  POLL_DATA, POLL_CONFIG, POLL_COMPLETE, APPEND};

extern uint8_t mac_buffer[6];
extern uint8_t MAC_ADDRESS_STA[6];
extern int ack_count;
extern int rej_count;
extern SemaphoreHandle_t xMutex_DataPoll; // mutex for LoRa hardware usage

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