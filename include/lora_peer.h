#ifndef LORA_PEER_H
#define LORA_PEER_H

#include <Arduino.h>

#define MAX_PEERS 30
#define MAC_ADDR_LENGTH 6

#define DEVICE_NAME_MAX_LENGTH 32

typedef struct Peer {
  uint8_t mac[MAC_ADDR_LENGTH];
  char deviceName[DEVICE_NAME_MAX_LENGTH];
}Peer;

extern Peer peers[MAX_PEERS];
extern size_t peerCount;

void printMacAddress(const uint8_t* mac);
bool addPeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH], String DeviceName);
bool removePeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH]);
bool checkPeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH]);
bool compareMacAddress(const uint8_t mac1[MAC_ADDR_LENGTH], const uint8_t mac2[MAC_ADDR_LENGTH]);
void savePeersToSD();
void loadPeersFromSD();
String getDeviceNameByMac(const uint8_t peer_addr[MAC_ADDR_LENGTH]);
bool isDeviceNameValid(String deviceName);

#endif