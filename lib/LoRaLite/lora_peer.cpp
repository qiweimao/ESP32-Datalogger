#include <Arduino.h>
#include <SD.h>
#include "lora_peer.h"

size_t peerCount = 0;
const char* filename = "/peers.txt";

Peer peers[MAX_PEERS];

// Function to print a MAC address
void printMacAddress(const uint8_t* mac) {
  for (int i = 0; i < MAC_ADDR_LENGTH; i++) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < MAC_ADDR_LENGTH - 1) Serial.print(":");
  }
}

// Function to add a peer gateway
bool addPeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH], String DeviceName) {
  if(checkPeerGateway(peer_addr)){
    Serial.println("Peer already added.");
    return false;
  }

  if (peerCount >= MAX_PEERS) {
    Serial.println("Max peers reached. Cannot add more.");
    return false;
  }
  
  for (size_t i = 0; i < MAC_ADDR_LENGTH; i++) {
    peers[peerCount].mac[i] = peer_addr[i];
  }
  DeviceName.toCharArray(peers[peerCount].deviceName, DEVICE_NAME_MAX_LENGTH);
  peerCount++;
  savePeersToSD();
  Serial.println("Peer saved to SD card amd list.");
  return true;
}

// Function to remove a peer gateway
bool removePeerGateway(const uint8_t peer_addr[MAC_ADDR_LENGTH]) {
  for (size_t i = 0; i < peerCount; i++) {
    bool match = true;
    for (size_t j = 0; j < MAC_ADDR_LENGTH; j++) {
      if (peers[i].mac[j] != peer_addr[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      for (size_t k = i; k < peerCount - 1; k++) {
        peers[k] = peers[k + 1];
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
      if (peers[i].mac[j] != peer_addr[j]) {
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
// Function to check if a peer gateway exists
int getIndexByMac(const uint8_t peer_addr[MAC_ADDR_LENGTH]) {
  for (size_t i = 0; i < peerCount; i++) {
    bool match = true;
    for (size_t j = 0; j < MAC_ADDR_LENGTH; j++) {
      if (peers[i].mac[j] != peer_addr[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      return i;
    }
  }
  return -1;
}

// Function to check if a peer gateway exists
bool isDeviceNameValid(String deviceName) {
  for (size_t i = 0; i < peerCount; i++) {
    if(deviceName.equals(peers[i].deviceName)){
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
      file.write(peers[i].mac, MAC_ADDR_LENGTH);
      file.write((uint8_t*)peers[i].deviceName, DEVICE_NAME_MAX_LENGTH);
    }
    file.close();
    Serial.print("Saved peers to SD card");
  }
}

// Function to load peers from SD card
void loadPeersFromSD() {
  
  if (!SD.exists(filename)) {
    Serial.println("File does not exist, skipping loading peers.");
    return;
  }

  File file = SD.open(filename, FILE_READ);
  if (file) {
    peerCount = 0;
    while (file.available() && peerCount < MAX_PEERS) {
      file.read(peers[peerCount].mac, MAC_ADDR_LENGTH);
      file.read((uint8_t*)peers[peerCount].deviceName, DEVICE_NAME_MAX_LENGTH);
      // Print loaded peer information to serial
      Serial.print("Loaded Peer ");
      Serial.print(peerCount + 1);
      Serial.print(":\n MAC = ");
      printMacAddress(peers[peerCount].mac);
      Serial.print(", Device Name = ");
      Serial.println(peers[peerCount].deviceName);
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

// Function to get device name by MAC address
String getDeviceNameByMac(const uint8_t peer_addr[MAC_ADDR_LENGTH]) {
  for (size_t i = 0; i < peerCount; i++) {
    if (compareMacAddress(peers[i].mac, peer_addr)) {
      return String(peers[i].deviceName);
    }
  }
  return String(""); // Return empty string if not found
}

// Function to get MAC address by device name
bool getMacByDeviceName(const String& deviceName, uint8_t* mac) {
  for (size_t i = 0; i < peerCount; i++) {
    if (deviceName.equals(peers[i].deviceName)) {
      memcpy(mac, peers[i].mac, MAC_ADDR_LENGTH); // Copy MAC address to the provided buffer
      return true; // Return true if device name is found
    }
  }
  return false; // Return false if device name is not found
}

