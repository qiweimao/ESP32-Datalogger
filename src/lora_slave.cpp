#include "lora_slave.h"
#include "lora_peer.h"
#include "lora_file_transfer.h"
#include "configuration.h"

/******************************************************************
 *                                                                *
 *                            Slave                               *
 *                                                                *
 ******************************************************************/

PairingStatus pairingStatus = NOT_PAIRED;

//Create 2 struct_message 
struct_message myData;  // data to send
struct_message inData;  // data received
struct_pairing pairingDataNode;

unsigned long currentMillis = millis();
unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 5000;        // Interval at which to publish sensor readings
unsigned long NodeStart;                // used to measure Pairing time
unsigned int readingId = 0;

void OnDataRecvNode(const uint8_t *incomingData, int len) { 
  uint8_t type = incomingData[0];
  Serial.printf("type = %d\n", type);

  // Check if message is for me
  uint8_t buffer[6];
  memcpy(buffer, incomingData + 1, 6);
  printMacAddress(buffer);
  if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
    Serial.println("This message is not for me.");
    return;
  };

  switch (type) {

  case PAIRING:    // we received pairing data from server
    Serial.println("\nPAIRING message processing");
    memcpy(&pairingDataNode, incomingData, sizeof(pairingDataNode));
    Serial.println();
    printMacAddress(pairingDataNode.mac);
    Serial.println();
    printMacAddress(MAC_ADDRESS_STA);
    Serial.println();
    if (compareMacAddress(pairingDataNode.mac, MAC_ADDRESS_STA)) {// Is this for me?
      Serial.print("Pairing done for ");
      Serial.print(" in ");
      Serial.print(millis()-NodeStart);
      Serial.println("ms");
      pairingStatus = PAIR_PAIRED;             // set the pairing status
    }
    else{
      Serial.print("Pairing mac address is not for me");
    }
    break;
  
  case ACK:
    ack_count++;
    break;
  }  
}

PairingStatus autoPairing(){
  switch(pairingStatus) {

    case PAIR_REQUEST:
    // set pairing data to send to the server
    Serial.println("\nBegin pairing Request");
    pairingDataNode.msgType = PAIRING;
    strncpy(pairingDataNode.deviceName, DEVICE_NAME.c_str(), sizeof(pairingDataNode.deviceName) - 1);
    pairingDataNode.deviceName[sizeof(pairingDataNode.deviceName) - 1] = '\0';
    memcpy(pairingDataNode.mac, MAC_ADDRESS_STA, sizeof(MAC_ADDRESS_STA));
    pairingDataNode.pairingKey = PAIRING_KEY;
    printMacAddress(pairingDataNode.mac);
    Serial.println();
    // add peer and send request
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
    if(currentMillis - previousMillis > 5000) {
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
      // Open the file in read mode
      // delay(15000);
      File file = SD.open("/collection_config");
      if (!file) {
        Serial.println("Failed to open file");
        return;
      }
      size_t fileSize = file.size();
      // sendMetadata("collection_config", fileSize);
      sendFile("/collection_config");
      delay(1000000);
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); // Add a small delay to yield the CPU
  }
}

void lora_slave_init() {
  NodeStart = millis();

  pairingStatus = PAIR_REQUEST;

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

  xTaskCreate(taskReceive, "Data Handler", 10000, (void *)OnDataRecvNode, 1, NULL);
  xTaskCreate(pairingTask, "Pairing Task", 10000, NULL, 1, NULL);

}