#include "lora_slave.h"
#include "lora_peer.h"
#include "lora_file_transfer.h"
#include "configuration.h"
#include "utils.h"


PairingStatus pairingStatus = NOT_PAIRED;
struct_pairing pairingDataNode;
uint8_t mac_master_paired[MAC_ADDR_LENGTH]; // identity for master node

unsigned long currentMillis = millis();
unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 5000;        // Interval at which to publish sensor readings
unsigned long NodeStart;                // used to measure Pairing time
unsigned int readingId = 0;

volatile bool sendFileRequest = false;
volatile bool sendConfigRequest = false;

/******************************************************************
 *                                                                *
 *                          Send Control                          *
 *                                                                *
 ******************************************************************/

// **************************************
// * Send One File From Folder
// **************************************
void send_files_to_gateway(String folderPath) {
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
      if (sendFile(fullFilePath.c_str(), SYNC)) {// append mode
        // String newFileName = fullFilePath.substring(0, fullFilePath.lastIndexOf('.')) + ".p";
        // SD.rename(fullFilePath, newFileName);
      }
      file.close();
      break; // Only send one file from each folder
    }
    file = root.openNextFile();
  }
  root.close();
}

void send_config_to_gateway() {
  File root = SD.open("/");

  if (!root) {
    Serial.println("Failed to open directory: /");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    if (!file.isDirectory() && fileName.endsWith(".conf")) {
      String fullFilePath = "/" + fileName;
      if (sendFile(fullFilePath.c_str(), SEND)) {
        Serial.println("Sent config file.");
      }
      file.close();
    }
    file = root.openNextFile();
  }
  root.close();
}

// **************************************
// * Task Send File
// **************************************
void sendFilesTask(void * parameter) {

  while(1){

    if(sendFileRequest){
            
      unsigned long startTime = millis();  // Start time

      Serial.println("Begin sending files");
      send_files_to_gateway("/data/ADC");
      send_files_to_gateway("/data/UART");
      send_files_to_gateway("/data/I2C");

      // send end of sync signal
      signal_message poll_complete_msg;
      memcpy(&poll_complete_msg.mac, MAC_ADDRESS_STA, MAC_ADDR_LENGTH);
      poll_complete_msg.msgType = POLL_COMPLETE;
      sendLoraMessage((uint8_t *)&poll_complete_msg, sizeof(poll_complete_msg));

      unsigned long endTime = millis();  // End time
      unsigned long elapsedTime = endTime - startTime;  // Calculate elapsed time

      Serial.println("Finished sending files");
      Serial.print("Time taken: ");
      Serial.print(elapsedTime);
      Serial.println(" ms");

      Serial.println("Finished sending files");
      sendFileRequest = false;
    }

    if(sendConfigRequest){
      
      send_config_to_gateway();

      // send end of sync signal
      signal_message poll_complete_msg;
      memcpy(&poll_complete_msg.mac, MAC_ADDRESS_STA, MAC_ADDR_LENGTH);
      poll_complete_msg.msgType = POLL_COMPLETE;
      sendLoraMessage((uint8_t *)&poll_complete_msg, sizeof(poll_complete_msg));

      sendConfigRequest = false;
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
        strncpy(pairingDataNode.deviceName, systemConfig.DEVICE_NAME, sizeof(pairingDataNode.deviceName) - 1); // device name
        pairingDataNode.deviceName[sizeof(pairingDataNode.deviceName) - 1] = '\0'; // device name null terminate
        memcpy(pairingDataNode.mac_origin, MAC_ADDRESS_STA, sizeof(MAC_ADDRESS_STA)); // device mac address
        pairingDataNode.pairingKey = systemConfig.PAIRING_KEY; // paring key for network

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
  Serial.println("Entered slave onreceive");

  // Check MAC address if message is for me
  uint8_t buffer[6];
  memcpy(buffer, incomingData + 1, 6);

  uint8_t type = incomingData[0];
  Serial.printf("type = %d\n", type);
  
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
    
    case POLL_DATA:
      if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
        Serial.println("This message is not for me.");
        return;
      };
      Serial.println("\nPOLL_DATA Received");
      sendFileRequest = true; // a flag to indicate that gateway requested data
      break;

    case POLL_CONFIG:
      Serial.println("\nPOLL_CONFIG Received");
      if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
        Serial.println("This message is not for me.");
        return;
      };
      sendConfigRequest = true; // a flag to indicate that gateway requested data
      break;
    
    case ACK:
      if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
        Serial.println("This message is not for me.");
        return;
      };
      ack_count++;
      break;

    case REJ:
      if(!compareMacAddress(buffer, MAC_ADDRESS_STA)){
        Serial.println("This message is not for me.");
        return;
      };
      rej_count++;
      break;
    
    case TIME_SYNC: {

      Serial.println("\nTime Sync Received");
      time_sync_message msg;
      memcpy(&msg, incomingData, sizeof(msg));

      // Prepare a struct tm object to hold the received time
      struct tm timeinfo;
      timeinfo.tm_year = msg.year - 1900; // years since 1900
      timeinfo.tm_mon = msg.month - 1;    // months since January (0-11)
      timeinfo.tm_mday = msg.day;         // day of the month (1-31)
      timeinfo.tm_hour = msg.hour;        // hours since midnight (0-23)
      timeinfo.tm_min = msg.minute;       // minutes after the hour (0-59)
      timeinfo.tm_sec = msg.second;       // seconds after the minute (0-61, allows for leap seconds)

      // Convert struct tm to time_t
      time_t epoch = mktime(&timeinfo);

      // Update system time using settimeofday
      struct timeval tv;
      tv.tv_sec = epoch;
      tv.tv_usec = 0;
      if (settimeofday(&tv, nullptr) != 0) {
        Serial.println("Failed to set system time.");
      } else {
        Serial.println("System time updated successfully.");
      }
      
      // update RTC Time
      external_rtc_sync_ntp();

      break;
    }

    default:
      Serial.println("Unknown message type");
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
  
  xTaskCreate(taskReceive, "Data Handler", 10000, (void *)OnDataRecvNode, 1, NULL); // register slave handler with receive task
  xTaskCreate(autoPairing, "Pairing Task", 10000, NULL, 1, NULL);
  xTaskCreate(sendFilesTask, "Send File Task", 10000, NULL, 1, NULL);

}