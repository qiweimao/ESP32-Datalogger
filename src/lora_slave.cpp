#include "lora_slave.h"
#include "lora_peer.h"
#include "lora_file_transfer.h"
#include "configuration.h"


PairingStatus pairingStatus = NOT_PAIRED;
struct_pairing pairingDataNode;
uint8_t mac_master_paired[MAC_ADDR_LENGTH]; // identity for master node

unsigned long currentMillis = millis();
unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 5000;        // Interval at which to publish sensor readings
unsigned long NodeStart;                // used to measure Pairing time
unsigned int readingId = 0;

volatile bool sendFileRequest = false;

/******************************************************************
 *                                                                *
 *                       Receive Control                          *
 *                                                                *
 ******************************************************************/

void send_files_to_gateway(String type) {
  String folderPath = "/data/" + type;
  File root = SD.open(folderPath);

  if (!root) {
    Serial.println("Failed to open directory: " + folderPath);
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    if (!file.isDirectory() && fileName.endsWith(".dat")) {
      String fullFilePath = folderPath + "/" + fileName;
      if (sendFile(fullFilePath.c_str())) {
        String newFileName = fullFilePath.substring(0, fullFilePath.lastIndexOf('.')) + ".p";
        SD.rename(fullFilePath, newFileName);
      }
      file.close();
      break; // Only send one file from each folder
    }
    file = root.openNextFile();
  }
  root.close();
}

void sendFilesTask(void * parameter) {
  while(1){
    if(sendFileRequest){
      send_files_to_gateway("ADC");
      send_files_to_gateway("UART");
      send_files_to_gateway("I2C");
      sendFileRequest = false;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // Delay for 1 second
  }
}


void OnDataRecvNode(const uint8_t *incomingData, int len) { 
  uint8_t type = incomingData[0];
  // Serial.printf("type = %d\n", type);

  // Check if message is for me
  uint8_t buffer[6];
  memcpy(buffer, incomingData + 1, 6);
  // printMacAddress(buffer);
  if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
    Serial.println("This message is not for me.");
    return;
  };

  switch (type) {

    case PAIRING:    // we received pairing data from server
    
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
    
    case POLL_DATA:
      sendFileRequest = true;
      break;
    
    case ACK:
      ack_count++;
      break;

    case REJ:
      rej_count++;
      break;
    default:
      Serial.println("Unknown message type");
  }

}

PairingStatus autoPairing(){
  switch(pairingStatus) {

    case PAIR_REQUEST:
    // set pairing data to send to the server
    Serial.println("\nBegin pairing Request");
    pairingDataNode.msgType = PAIRING;
    strncpy(pairingDataNode.deviceName, systemConfig.DEVICE_NAME, sizeof(pairingDataNode.deviceName) - 1);
    pairingDataNode.deviceName[sizeof(pairingDataNode.deviceName) - 1] = '\0';
    memcpy(pairingDataNode.mac_origin, MAC_ADDRESS_STA, sizeof(MAC_ADDRESS_STA));
    pairingDataNode.pairingKey = systemConfig.PAIRING_KEY;

    printMacAddress(pairingDataNode.mac_origin);Serial.println();
    Serial.println(pairingDataNode.deviceName);

    LoRa.beginPacket();
    LoRa.write((uint8_t *) &pairingDataNode, sizeof(pairingDataNode));
    LoRa.endPacket();
    LoRa.receive();
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
      // nothing to do here 
      // Serial.println("Paired");
    break;
  }
  return pairingStatus;
}  

void pairingTask(void *pvParameters) {
  while(true){
    if (autoPairing() == PAIR_PAIRED) {
      // // Open the file in read mode
      // // delay(15000);
      // File file = SD.open("/collection_config");
      // if (!file) {
      //   Serial.println("Failed to open file");
      //   return;
      // }
      // size_t fileSize = file.size();
      // sendFile("/collection_config");
      // delay(1000000);
    }
    delay(1000); // Add a small delay to yield the CPU
  }
}

void lora_slave_init() {
  NodeStart = millis();

  pairingStatus = PAIR_REQUEST;

  LoRa.onReceive(onReceive);
  // LoRa.onTxDone(onTxDone);
  // LoRa_rxMode();
  LoRa.receive();

  xTaskCreate(taskReceive, "Data Handler", 10000, (void *)OnDataRecvNode, 1, NULL);
  xTaskCreate(pairingTask, "Pairing Task", 10000, NULL, 1, NULL);
  xTaskCreate(sendFilesTask, "Send File Task", 10000, NULL, 1, NULL);

}