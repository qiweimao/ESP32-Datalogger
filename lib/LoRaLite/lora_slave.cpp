#include "lora_slave.h"
#include "lora_peer.h"
#include "lora_init.h"
#include "lora_file_transfer.h"
#include "SD.h"

PairingStatus pairingStatus = NOT_PAIRED;
struct_pairing pairingDataNode;
signal_message signal_msg;
uint8_t mac_master_paired[MAC_ADDR_LENGTH]; // identity for master node

unsigned long currentMillis = millis();
unsigned long previousMillis = 0;   // Stores last time temperature was published
unsigned long NodeStart;                // used to measure Pairing time

volatile bool syncFolderRequest = false;
volatile bool fileRequest = false;

String sync_folder_path = "";
String sync_extension = "";
String file_path = "";

/******************************************************************
 *                                                                *
 *                          Send Control                          *
 *                                                                *
 ******************************************************************/

// **************************************
// * Send All .dat File From Folder
// **************************************
int sync_folder(String folderPath, String extension) {
  File root = SD.open(folderPath);

  if (!root) {
    Serial.println("Failed to open directory: " + folderPath);
    return -1;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    if (!file.isDirectory() && fileName.endsWith(extension)) {
      String fullFilePath = folderPath + "/" + fileName;
      if (!sendLoRaFile(fullFilePath.c_str(), SYNC)) {// append mode
        Serial.println("sync_folder: send file failed, abort.");
        file.close();
        return -2;
      }
      file.close();
    }
    file = root.openNextFile();
  }
  root.close();
  return 0;
}

void send_file(String path) {

  Serial.println("=== Sending Single File ===");
  if(sendLoRaFile(path.c_str(), SEND)) {
    Serial.println("Sent data collection configuration to gateway.");
  }

}

// **************************************
// * Task Send File
// **************************************
void sendFilesTask(void * parameter) {

  while(1){

    if(syncFolderRequest){
            
      unsigned long startTime = millis();  // Start time

      Serial.println();Serial.println("=== SYNC_FOLDER ===");
      if (sync_folder(sync_folder_path, sync_extension) == 0){
        // send end of sync signal
        signal_message poll_complete_msg;
        memcpy(&poll_complete_msg.mac, MAC_ADDRESS_STA, MAC_ADDR_LENGTH);
        poll_complete_msg.msgType = POLL_COMPLETE;
        sendLoraMessage((uint8_t *)&poll_complete_msg, sizeof(poll_complete_msg));
      }
      else{
        Serial.println("sendFilesTask: sync_folder fail.");
      }


      unsigned long endTime = millis();  // End time
      unsigned long elapsedTime = endTime - startTime;  // Calculate elapsed time

      Serial.println("Finished sending files");
      Serial.print("Time taken: ");
      Serial.print(elapsedTime);
      Serial.println(" ms");

      Serial.println("Finished sending files");
      syncFolderRequest = false;
    }

    if(fileRequest){
      
      Serial.println("=== Send config structs ===");
      send_file(file_path);

      // send end of sync signal
      signal_message poll_complete_msg;
      memcpy(&poll_complete_msg.mac, MAC_ADDRESS_STA, MAC_ADDR_LENGTH);
      poll_complete_msg.msgType = POLL_COMPLETE;
      sendLoraMessage((uint8_t *)&poll_complete_msg, sizeof(poll_complete_msg));

      fileRequest = false;
    }

    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

// **************************************
// * Pairing Task
// **************************************
void autoPairing(void * parameter){
  while(true){

    switch(pairingStatus) {

      case PAIR_REQUEST:

        Serial.println("\nBegin pairing Request");
        pairingDataNode.msgType = PAIRING; // message type
        memcpy(pairingDataNode.mac_origin, MAC_ADDRESS_STA, sizeof(MAC_ADDRESS_STA)); // device mac address
        pairingDataNode.pairingKey = lora_config.pairingKey; // paring key for network
        memcpy(pairingDataNode.deviceName, lora_config.deviceName.c_str() ,sizeof(pairingDataNode.deviceName));

        printMacAddress(pairingDataNode.mac_origin);Serial.println();
        Serial.println(pairingDataNode.deviceName);

        sendLoraMessage((uint8_t *) &pairingDataNode, sizeof(pairingDataNode));

        previousMillis = millis();
        pairingStatus = PAIR_REQUESTED;
        Serial.println("Pairing request sent\n");

        break;

      case PAIR_REQUESTED:
        // time out to allow receiving response from server
        currentMillis = millis();
        if(currentMillis - previousMillis > ACK_TIMEOUT) {
          previousMillis = currentMillis;
          // time out expired,  try next channel
          pairingStatus = PAIR_REQUEST;
          Serial.println("Request again...");
        }
        break;

      case PAIR_PAIRED:
        break;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second

  }
} 

/******************************************************************
 *                                                                *
 *                         Receive Control                        *
 *                                                                *
 ******************************************************************/

void OnDataRecvNode(const uint8_t *incomingData, int len) { 

  // Check MAC address if message is for me
  uint8_t buffer[6];
  memcpy(buffer, incomingData + 1, 6);

  uint8_t type = incomingData[0];

  // Serial.print("OnDataRecvNode, type: "); Serial.println(type);
  
  // User registered callback
  LoRaMessageHandlerFunc handler = findHandler(&lora_config, type, 1); // 1 indicates slave
  if (handler) {
      // Call the registered handler
      handler(incomingData);
      return;
  }

  // library predefined handler
  switch (type) {

    case PAIRING:    // we received pairing data from server

      if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
        Serial.println("This message is not for me.");
        return;
      };

      Serial.println("\nPAIRING ACK Received");
      memcpy(&pairingDataNode, incomingData, sizeof(pairingDataNode));
      Serial.println("Master MAC:");
      printMacAddress(pairingDataNode.mac_master);
      memcpy(mac_master_paired, pairingDataNode.mac_master, sizeof(pairingDataNode.mac_master));
      Serial.println();
      Serial.print("Pairing done ");
      Serial.print(" in ");
      Serial.print(millis()-NodeStart);
      Serial.println("ms\n");
      pairingStatus = PAIR_PAIRED;

      break;
    
    case SYNC_FOLDER:

      if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
        Serial.println("This message is not for me.");
        return;
      };

      Serial.println("SYNC_FOLDER Received");

      memcpy(&signal_msg, incomingData, sizeof(signal_msg));
      sync_folder_path = signal_msg.path;
      sync_extension = signal_msg.extension;
      syncFolderRequest = true; // a flag to indicate that gateway requested data

      break;

    case GET_FILE:
      Serial.println("\nGET_FILE Received");
      if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
        Serial.println("This message is not for me.");
        return;
      };

      memcpy(&signal_msg, incomingData, sizeof(signal_msg));
      file_path = signal_msg.path;
      fileRequest = true; // a flag to indicate that gateway requested data

      break;
    
    case ACK:
      if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
        Serial.println("This message is not for me.");
        return;
      };
      // Serial.println("Received ACK");
      ack_count++;
      break;

    case REJ:
      if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
        Serial.println("This message is not for me.");
        return;
      };
      Serial.println("Received REJ");
      rej_count++;
      break;
    

    default:

      Serial.print("Unknown message:"); Serial.println(type);
      break;
  }

}


/******************************************************************
 *                                                                *
 *                              Init                              *
 *                                                                *
 ******************************************************************/

void lora_slave_init() {

  LoRa.onReceive(onReceive); // this just increment the data receive counter
  LoRa.receive();

  NodeStart = millis();
  pairingStatus = PAIR_REQUEST;
  
  xTaskCreate(taskReceive, "Data Handler", 10000, (void *)OnDataRecvNode, 2, NULL); // register slave handler with receive task
  xTaskCreate(autoPairing, "Pairing Task", 10000, NULL, 1, NULL);
  xTaskCreate(sendFilesTask, "Send File Task", 10000, NULL, 1, NULL);

}