#ifndef LORA_INIT_H
#define LORA_INIT_H

#include <LoRa.h>
#include "lora_peer.h"

#define LORA_SLAVE 0
#define LORA_GATEWAY 1

#define MAX_DEVICE_NAME_LEN 10  
#define MAX_FILENAME_LEN 20
#define MAX_JSON_LEN_1 20
#define MAX_JSON_LEN_2 10

#define MAX_HANDLERS 20
#define MAX_SCHEDULES 20
#define MAX_ATTEMPS 1 // only attempt at gateway's request

enum PairingStatus {NOT_PAIRED, PAIR_REQUEST, PAIR_REQUESTED, PAIR_PAIRED,};
enum MessageType {PAIRING, \
                  FILE_BODY, FILE_ENTIRE, ACK, REJ, TIMEOUT, 
                  SYNC_FOLDER, GET_FILE, POLL_COMPLETE,
                  USER_DEFINED_START = 100  // Reserve range for user-defined types
};
#include "lora_user_settings.h"


typedef void (*DataRecvCallback)(const uint8_t *incomingData, int len);
typedef void (*LoRaMessageHandlerFunc)(const uint8_t *incomingData);
typedef void (*LoRaPollFunc)(int peer_index); // poll file or dir

typedef struct {
    uint8_t messageType;
    LoRaMessageHandlerFunc slave;
    LoRaMessageHandlerFunc gateway;
} LoRaMessageHandler;

typedef struct {
    LoRaPollFunc func;
    int isBroadcast;
    unsigned long lastPoll;
    unsigned long interval;
} LoRaPollSchedule;

typedef struct {
    int lora_mode;
    String deviceName;
    uint32_t pairingKey;
    LoRaMessageHandler messageHandlers[MAX_HANDLERS];
    LoRaPollSchedule schedules[MAX_SCHEDULES];
    int handlerCount = 0;
    int scheduleCount = 0;
} LoRaConfig;

int addHandler(LoRaConfig *config, uint8_t messageType, LoRaMessageHandlerFunc handlerFunc_slave, LoRaMessageHandlerFunc handlerFunc_gateway);
int addSchedule(LoRaConfig *config, LoRaPollFunc func, unsigned long interval, int isBroadcast);
LoRaMessageHandlerFunc findHandler(LoRaConfig *config, uint8_t messageType, int isSlave);

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
  char path[MAX_FILENAME_LEN]; // can be folder or full path to a file
  char extension[MAX_FILENAME_LEN];
} signal_message;

typedef struct sysconfig_message {
  uint8_t msgType;
  uint8_t mac[MAC_ADDR_LENGTH];
  char key[MAX_JSON_LEN_1];
  char value[MAX_JSON_LEN_1];
} sysconfig_message;

typedef struct collection_config_message {
  uint8_t msgType;
  uint8_t mac[MAC_ADDR_LENGTH];
  int channel;
  int pin;
  int sensor;
  bool enabled;
  uint16_t interval;
} collection_config_message;

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


extern uint8_t mac_buffer[6];
extern uint8_t MAC_ADDRESS_STA[6];
extern int ack_count;
extern int rej_count;
extern bool rej_switch;//Flag to indicate if the gateway would send a REJ to incoming file body
extern SemaphoreHandle_t xMutex_DataPoll; // mutex for LoRa hardware usage
extern SemaphoreHandle_t xMutex_LoRaHardware; // mutex for LoRa hardware usage
extern LoRaConfig lora_config;

void onReceive(int packetSize);
void taskReceive(void *parameter);
void sendLoraMessage(uint8_t* data, size_t size);
void handle_pairing(const uint8_t *incomingData);
void lora_init(LoRaConfig *config);



#endif