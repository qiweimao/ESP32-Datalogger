#include <ArduinoJson.h>

#include "utils.h"
#include "lora_init.h"

struct_message incomingReadings;
struct_message outgoingSetpoints;
struct_pairing pairingDataGateway;

int counter = 0;
void readDataToSend() {
  outgoingSetpoints.msgType = DATA;
  outgoingSetpoints.id = 0;
  outgoingSetpoints.temp = random(0, 40);
  outgoingSetpoints.hum = random(0, 100);
  outgoingSetpoints.readingId = counter++;
}

// Define the maximum number of peers
#define MAX_PEERS 10

// Define the length of the peer address
#define PEER_ADDR_LENGTH 6

// Array to store peer addresses
uint8_t peers[MAX_PEERS];

// Counter to keep track of the number of peers
int peerCount = 0;

// Function to add a peer gateway
bool addPeerGateway(const uint8_t peer_addr) {
  // Check if we have space for more peers
  if (peerCount >= MAX_PEERS) {
    Serial.println("Max peers reached. Cannot add more.");
    return false;
  }
  
  // Copy the peer address to the peers array
  peers[peerCount] = peer_addr;

  // Increment the peer count
  peerCount++;

  Serial.println();

  return true;
}


void OnDataRecvGateway(const uint8_t *incomingData, int len) { 
  Serial.print(len);
  Serial.println();
  StaticJsonDocument<1000> root;
  String payload;
  uint8_t type = incomingData[0];       // first message byte is the type of message 
  Serial.printf("Incoming data type: unit8_t = ");
  Serial.println(type);
  switch (type) {
    case DATA :                           // the message is data type
      Serial.println("Data type: DATA");
      oled_print("Data type: DATA");

      memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
      // create a JSON document with received data and send it by event to the web page
      root["id"] = incomingReadings.id;
      root["temperature"] = incomingReadings.temp;

      root["humidity"] = incomingReadings.hum;
      root["readingId"] = String(incomingReadings.readingId);
      serializeJson(root, payload);
      Serial.print("event send :");
      serializeJson(root, Serial);
      Serial.println();
      break;
    
    case PAIRING:                            // the message is a pairing request 
      Serial.println("Data type: PAIRING");
      oled_print("Data type: PAIRING");

      memcpy(&pairingDataGateway, incomingData, sizeof(pairingDataGateway));

      Serial.println(pairingDataGateway.msgType);
      Serial.println(pairingDataGateway.id);
      Serial.print("Pairing request from: ");

      /* OLED for Dev */
      oled_print(pairingDataGateway.msgType);
      oled_print(pairingDataGateway.id);
      oled_print("Pairing request from: ");

      if (pairingDataGateway.id > 0) {     // do not replay to server itself
        if (pairingDataGateway.msgType == PAIRING) { 
            pairingDataGateway.id = 0;       // 0 is server
            Serial.println("send response");
            oled_print("send response");
            Serial.println("Sending packet...");
            LoRa.beginPacket();
            LoRa.write((uint8_t *) &pairingDataGateway, sizeof(pairingDataGateway));
            LoRa.endPacket();
            addPeerGateway(pairingDataGateway.id);
        }  
      }  
      break; 
  }
}

void GatewayInit() {

  // Serial.println();
  // Serial.print("Server MAC Address:  ");
  // Serial.println(WiFi.macAddress());

  // // Set the device as a Station and Soft Access Point simultaneously
  // WiFi.mode(WIFI_AP_STA);
  // // Set device as a Wi-Fi Station
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Serial.println(WIFI_SSID);
  // Serial.println(WIFI_PASSWORD);
  // int i = 0;
  // while (WiFi.status() != WL_CONNECTED && i < 5) {
  //   i++;
  //   delay(1000);
  //   Serial.println("Setting as a Wi-Fi Station..");
  // }
  // String ssid = "Gateway";
  // String password = "helloworld";
  // WiFi.softAP(ssid, password);

  // Serial.print("Server SOFT AP MAC Address:  ");
  // Serial.println(WiFi.softAPmacAddress());

  // gateway_channel = WiFi.channel();
  // Serial.print("Station IP Address: ");
  // Serial.println(WiFi.localIP());
  // Serial.print("Wi-Fi Channel: ");
  // Serial.println(WiFi.channel());

  // // Init ESP-NOW
  // if (esp_now_init() != ESP_OK) {
  //   Serial.println("Error initializing ESP-NOW");
  //   return;
  // }
  // esp_now_register_send_cb(OnDataSent);
  // esp_now_register_recv_cb(OnDataRecvGateway);  
}

// void pingNode() {
//   static unsigned long lastEventTime = millis();
//   static const unsigned long EVENT_INTERVAL_MS = 5000;
//   if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
//     lastEventTime = millis();
//     readDataToSend();
//     esp_now_send(NULL, (uint8_t *) &outgoingSetpoints, sizeof(outgoingSetpoints));
//   }
// }