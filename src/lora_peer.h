#include <Arduino.h>

#define MAX_PEERS 10
#define MAC_ADDR_LENGTH 8

extern uint8_t peers[MAX_PEERS][MAC_ADDR_LENGTH];

void printMacAddress(const uint8_t* mac);
bool addPeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH]);
bool removePeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH]);
bool checkPeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH]);
bool compareMacAddress(const uint8_t mac1[MAC_ADDR_LENGTH], const uint8_t mac2[MAC_ADDR_LENGTH]);
void savePeersToSD();
void loadPeersFromSD();