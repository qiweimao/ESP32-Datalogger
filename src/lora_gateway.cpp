#include "lora_gateway.h"
#include "lora_peer.h"
#include "configuration.h"
#include "utils.h"

struct_message incomingReadings;
struct_pairing pairingDataGateway;
file_meta_message file_meta_gateway;
ack_message ackMessage_gateway;
file_body_message file_body_gateway;
String current_file_path;
file_end_message file_end_gateway;
size_t total_bytes_received;
size_t total_bytes_written;
size_t bytes_written;

File file;

void OnDataRecvGateway(const uint8_t *incomingData, int len) { 
  JsonDocument root;
  String payload;
  uint8_t type = incomingData[0];       // first message byte is the type of message 

  switch (type) {
    
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

    case FILE_BODY:
      
      Serial.println("Received FILE_BODY.");
      memcpy(&file_body_gateway, incomingData, sizeof(file_body_gateway));
      file = SD.open(current_file_path, FILE_APPEND);
      if (!file) {
        Serial.println("Failed to create file");
        return;
      }

      // Write the file_body_gateway structure to the file
      Serial.printf("Received %d bytes\n", file_body_gateway.len);
      total_bytes_received += file_body_gateway.len;

      bytes_written = file.write((const uint8_t*)&file_body_gateway.data, file_body_gateway.len);
      total_bytes_written += bytes_written;
      Serial.printf("Wrote %d bytes\n", bytes_written);
      
      file.close();

      ackMessage_gateway.msgType = ACK;
      memcpy(&ackMessage_gateway.mac, file_meta_gateway.mac, sizeof(file_meta_gateway.mac));
      sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));

      Serial.println("Data written to file successfully");

      break;


    case FILE_META:

      memcpy(&file_meta_gateway, incomingData, sizeof(file_meta_gateway));

      // Get File Name
      char buffer[MAX_FILENAME_LEN + 1]; // 10 for the name + 1 for the null terminator
      strncpy(buffer, file_meta_gateway.filename, MAX_FILENAME_LEN);
      buffer[MAX_FILENAME_LEN] = '\0'; // Ensure null-termination
      char filename[MAX_FILENAME_LEN + 2]; // 1 for '/' + 4 for 'data' + 1 for '/' + 10 for the name + 1 for the null terminator
      snprintf(filename, sizeof(filename), "%s", buffer);

      Serial.println("\nRecieved FILE_META");
      Serial.println(filename);
      Serial.printf("File size: %d bytes.\n",file_meta_gateway.filesize);

      // Get Node Name

      current_file_path = "/node/" + getDeviceNameByMac(file_meta_gateway.mac) + String(filename);
      Serial.println(current_file_path);

      // Create and open an empty file
      file = SD.open(current_file_path, FILE_WRITE);
      if (!file) {
        Serial.println("Failed to create file");
        return;
      }
      
      Serial.println("File created successfully");

      // Close the file
      file.close();
      Serial.println("File closed successfully");
      
      ackMessage_gateway.msgType = ACK;
      memcpy(&ackMessage_gateway.mac, file_meta_gateway.mac, sizeof(ackMessage_gateway.mac));
      sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));

      total_bytes_received = 0;

      break;

    case FILE_END:

      memcpy(&file_end_gateway, incomingData, sizeof(file_end_gateway));
      Serial.print("\nReceived FILE_END from:");
      printMacAddress(file_end_gateway.mac); Serial.println();
      Serial.print("Total bytes received: "); Serial.println(total_bytes_received);

      ackMessage_gateway.msgType = ACK;
      memcpy(&ackMessage_gateway.mac, file_end_gateway.mac, sizeof(ackMessage_gateway.mac));
      sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));

      break;

    default:
    
      Serial.print("This message is not for me.");

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