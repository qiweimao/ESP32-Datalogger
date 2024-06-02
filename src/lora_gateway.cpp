#include "lora_gateway.h"
#include "lora_peer.h"
#include "configuration.h"
#include "utils.h"

struct_message incomingReadings;
struct_pairing pairingDataGateway;
file_meta_message file_meta_gateway;

void OnDataRecvGateway(const uint8_t *incomingData, int len) { 
  JsonDocument root;
  String payload;
  uint8_t type = incomingData[0];       // first message byte is the type of message 
  switch (type) {

    case DATA :                         // the message is data type
      memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
      root["mac"] = incomingReadings.mac;      // create a JSON document with received data and send it by event to the web page
      root["temperature"] = incomingReadings.temp;
      root["humidity"] = incomingReadings.hum;
      root["readingId"] = String(incomingReadings.readingId);
      serializeJson(root, payload);
      oled_print(payload.c_str(), sizeof(payload.c_str()));
      serializeJson(root, Serial);
      Serial.println();
      break;
    
    case PAIRING:                            // the message is a pairing request 

      memcpy(&pairingDataGateway, incomingData, sizeof(pairingDataGateway));

      Serial.print("\nPairing request from: ");
      printMacAddress(pairingDataGateway.mac);
      Serial.println();
      Serial.println(pairingDataGateway.deviceName);

      /* OLED for Dev */
      oled_print("Pair request: ");
      // oled_print(pairingDataGateway.mac[0]);

      if (pairingDataGateway.msgType == PAIRING) { 

        if(pairingDataGateway.pairingKey == PAIRING_KEY){

          Serial.println("Correct PAIRING_KEY");

          oled_print("send response");
          sendLoraMessage((uint8_t *) &pairingDataGateway, sizeof(pairingDataGateway));
          Serial.println("Sent pairing response...");

          // If first time pairing, create a dir for this node
          if(addPeerGateway(pairingDataGateway.mac, pairingDataGateway.deviceName)){
            
            Serial.println("First time pairing, create a dir for this node");

            char deviceFolder[MAX_DEVICE_NAME_LEN + 1]; // 10 for the name + 1 for the null terminator
            strncpy(deviceFolder, pairingDataGateway.deviceName, 10);
            deviceFolder[MAX_DEVICE_NAME_LEN] = '\0'; // Ensure null-termination

            char folderPath[MAX_DEVICE_NAME_LEN + 6]; // 1 for '/' + 4 for 'data' + 1 for '/' + 10 for the name + 1 for the null terminator
            snprintf(folderPath, sizeof(folderPath), "/node/%s", deviceFolder);
            Serial.println(folderPath);

            if (SD.mkdir(folderPath)) {
              Serial.println("Directory created successfully.");
            } else {
              Serial.println("Failed to create directory.");
            }

            Serial.println("End of dir creation.");
          }

        }
        else{
          Serial.println("Wrong PAIRING_KEY");
        }
      }  

      break; 

    case FILE_META:
      memcpy(&file_meta_gateway, incomingData, sizeof(file_meta_gateway));

      // Get File Name
      char buffer[MAX_FILENAME_LEN + 1]; // 10 for the name + 1 for the null terminator
      strncpy(buffer, file_meta_gateway.filename, MAX_FILENAME_LEN);
      buffer[MAX_FILENAME_LEN] = '\0'; // Ensure null-termination

      char filename[MAX_FILENAME_LEN + 2]; // 1 for '/' + 4 for 'data' + 1 for '/' + 10 for the name + 1 for the null terminator
      snprintf(filename, sizeof(filename), "%s", buffer);
      Serial.println(filename);

      

  }
}

void lora_gateway_init() {

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

  loadPeersFromSD();

  // Ensure the "data" directory exists
  if (!SD.exists("/node")) {
    Serial.print("Creating 'node' directory - ");
    if (SD.mkdir("/node")) {
      Serial.println("OK");
    } else {
      Serial.println("Failed to create 'node' directory");
    }
  }
  
  xTaskCreate(taskReceive, "Data Handler", 10000, (void*)OnDataRecvGateway, 1, NULL);

}