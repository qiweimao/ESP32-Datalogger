#include <Arduino.h>
#include <SD.h>
#include "lora_peer.h"

size_t peerCount = 0;
const char* filename = "/peers.txt";

uint8_t peers[MAX_PEERS][MAC_ADDR_LENGTH];

// Function to print a MAC address
void printMacAddress(const uint8_t* mac) {
  for (int i = 0; i < MAC_ADDR_LENGTH; i++) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < MAC_ADDR_LENGTH - 1) Serial.print(":");
  }
}

// Function to add a peer gateway
bool addPeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH]) {
  if(checkPeerGateway(peer_addr)){
    Serial.println("Peer already added.");
    return false;
  }

  if (peerCount >= MAX_PEERS) {
    Serial.println("Max peers reached. Cannot add more.");
    return false;
  }
  for (size_t i = 0; i < MAC_ADDR_LENGTH; i++) {
    peers[peerCount][i] = peer_addr[i];
  }
  peerCount++;
  savePeersToSD();
  return true;
}

// Function to remove a peer gateway
bool removePeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH]) {
  for (size_t i = 0; i < peerCount; i++) {
    bool match = true;
    for (size_t j = 0; j < MAC_ADDR_LENGTH; j++) {
      if (peers[i][j] != peer_addr[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      for (size_t k = i; k < peerCount - 1; k++) {
        memcpy(peers[k], peers[k + 1], MAC_ADDR_LENGTH);
      }
      peerCount--;
      savePeersToSD();
      return true;
    }
  }
  return false;
}

// Function to check if a peer gateway exists
bool checkPeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH]) {
  for (size_t i = 0; i < peerCount; i++) {
    bool match = true;
    for (size_t j = 0; j < MAC_ADDR_LENGTH; j++) {
      if (peers[i][j] != peer_addr[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return true;
    }
  }
  return false;
}

// Function to save peers to SD card
void savePeersToSD() {
  File file = SD.open(filename, FILE_WRITE);
  if (file) {
    file.seek(0); // Move to the beginning of the file
    for (size_t i = 0; i < peerCount; i++) {
      for (size_t j = 0; j < MAC_ADDR_LENGTH; j++) {
        file.write(peers[i][j]);
      }
    }
    file.close();
  }
}

// Function to load peers from SD card
void loadPeersFromSD() {
  File file = SD.open(filename, FILE_READ);
  if (file) {
    peerCount = 0;
    while (file.available() && peerCount < MAX_PEERS) {
      for (size_t j = 0; j < MAC_ADDR_LENGTH; j++) {
        peers[peerCount][j] = file.read();
      }
      peerCount++;
    }
    file.close();
  }
}

// Function to compare two MAC addresses
bool compareMacAddress(const uint8_t mac1[MAC_ADDR_LENGTH], const uint8_t mac2[MAC_ADDR_LENGTH]) {
  for (size_t i = 0; i < MAC_ADDR_LENGTH; i++) {
    if (mac1[i] != mac2[i]) {
      return false; // If any byte doesn't match, return false
    }
  }
  return true; // If all bytes match, return true
}