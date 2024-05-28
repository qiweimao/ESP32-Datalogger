#include "lora_gateway.h"
#include "lora_peer.h"

struct_message incomingReadings;
struct_message outgoingSetpoints;
struct_pairing pairingDataGateway;

void OnDataRecvGateway(const uint8_t *incomingData, int len) { 
  JsonDocument root;
  String payload;
  uint8_t type = incomingData[0];       // first message byte is the type of message 
  switch (type) {
    case DATA :                           // the message is data type
      memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
      root["mac"] = incomingReadings.mac;      // create a JSON document with received data and send it by event to the web page
      root["temperature"] = incomingReadings.temp;
      root["humidity"] = incomingReadings.hum;
      root["readingId"] = String(incomingReadings.readingId);
      serializeJson(root, payload);
      oled_print(payload.c_str());
      serializeJson(root, Serial);
      Serial.println();
      break;
    
    case PAIRING:                            // the message is a pairing request 
      memcpy(&pairingDataGateway, incomingData, sizeof(pairingDataGateway));

      Serial.println(pairingDataGateway.msgType);
      Serial.print("Pairing request from: ");
      printMacAddress(pairingDataGateway.mac);

      /* OLED for Dev */
      oled_print(pairingDataGateway.msgType);
      oled_print(pairingDataGateway.mac[0]);
      oled_print("Pairing request from: ");

      if (pairingDataGateway.mac > 0) {
        if (pairingDataGateway.msgType == PAIRING) { 
            esp_read_mac(mac_buffer, ESP_MAC_WIFI_STA);  
            memcpy(pairingDataGateway.mac, mac_buffer, sizeof(mac_buffer));
            Serial.println("send response");
            oled_print("send response");
            Serial.println("Sending packet...");
            LoRa.beginPacket();
            LoRa.write((uint8_t *) &pairingDataGateway, sizeof(pairingDataGateway));
            LoRa.endPacket();
            addPeerGateway(pairingDataGateway.mac);
            LoRa.receive();
        }  
      }  
      break; 
  }
}

void lora_gateway_init() {

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

  xTaskCreate(taskReceive, "Data Handler", 10000, (void*)OnDataRecvGateway, 1, NULL);

}