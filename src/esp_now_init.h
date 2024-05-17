#ifndef ESP_NOW_INIT_H
#define ESP_NOW_INIT_H

#include <esp_now.h>
#include "esp_now_gateway.h"
#include "esp_now_node.h"

#define SAVE_CHANNEL

//Structure to send data
//Must match the receiver structure
// Structure example to receive data
// Must match the sender structure
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
    uint8_t macAddr[6];
    uint8_t channel;
} struct_pairing;

enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};

enum MessageType {PAIRING, DATA,};

extern int node_send_fail_count;

void esp_now_setup();
void printMAC(const uint8_t * mac_addr);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

#endif