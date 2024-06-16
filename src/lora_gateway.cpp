#include "lora_gateway.h"
#include "lora_file_transfer.h"
#include "lora_peer.h"
#include "configuration.h"
#include "utils.h"

unsigned long lastPollTime = 0;
unsigned long lastTimeSyncTime = 0;
unsigned long lastConfigSyncTime = 0;
const unsigned long pollInterval = 60000; // 1 minute
const unsigned long timeSyncInterval = 3600000;
const unsigned long configSyncInterval = 3600000;

bool apiTriggered = false;
bool peer_ack[MAX_PEERS];
bool poll_success = false;

/******************************************************************
 *                                                                *
 *                        Receive Control                         *
 *                                                                *
 ******************************************************************/

// ***********************
// * Handle Pairing
// ***********************
void handle_pairing(const uint8_t *incomingData){

  struct_pairing pairingDataGateway;
  memcpy(&pairingDataGateway, incomingData, sizeof(pairingDataGateway));

  Serial.print("\nPairing request from: ");
  printMacAddress(pairingDataGateway.mac_origin);
  Serial.println();
  Serial.println(pairingDataGateway.deviceName);
  oled_print("Pair request: ");

  // Check if has correct pairing key
  if(pairingDataGateway.pairingKey == systemConfig.PAIRING_KEY){
    Serial.println("Correct PAIRING_KEY");
    oled_print("send response");
    memcpy(&pairingDataGateway.mac_master, MAC_ADDRESS_STA, sizeof(MAC_ADDRESS_STA));
    sendLoraMessage((uint8_t *) &pairingDataGateway, sizeof(pairingDataGateway));
    Serial.println("Sent pairing response...");
  }
  else{
    Serial.println("Wrong PAIRING_KEY");
    return;
  }

  // If first time pairing, create a dir for this node
  if(addPeerGateway(pairingDataGateway.mac_origin, pairingDataGateway.deviceName)){
    
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

// *************************************
// * OnReceive Handlers Registration
// *************************************
void OnDataRecvGateway(const uint8_t *incomingData, int len) { 
  
  uint8_t type = incomingData[0];       // first message byte is the type of message 

  switch (type) {
    case PAIRING:                            // the message is a pairing request 
      handle_pairing(incomingData);
      break;
    case FILE_BODY:
      handle_file_body(incomingData);
      break;
    case FILE_META:
      handle_file_meta(incomingData);
      break;
    case FILE_END:
      if(!handle_file_end(incomingData)){
        poll_success = true;
      }
      break;
    default:
      Serial.println("Unkown message type.");
  }
}

/******************************************************************
 *                                                                *
 *                         Send Control                           *
 *                                                                *
 ******************************************************************/


int waitForPollAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < ACK_TIMEOUT) {
    if(poll_success){
      poll_success = false;
      return true;
    }
    vTaskDelay(20 / portTICK_PERIOD_MS); // Delay for 1 second
  }
  return false;
}

// ***********************
// * Poll Data
// ***********************

poll_data_message poll_data_struct(uint8_t *mac) {
  poll_data_message msg;
  msg.msgType = POLL_DATA;
  memcpy(&msg.mac, mac, MAC_ADDR_LENGTH);
  return msg;
}

void poll_data(uint8_t *mac){
  
  if (xSemaphoreTake(xMutex_DataPoll, portMAX_DELAY) == pdTRUE) {
    poll_success = false;
    poll_data_message msg = poll_data_struct(mac);
    sendLoraMessage((uint8_t *) &msg, sizeof(msg));
    Serial.printf("Sent data poll message to:");
    printMacAddress(mac);Serial.println();Serial.println();
    waitForPollAck(); // check for ack before proceeding to next one
    xSemaphoreGive(xMutex_DataPoll);
  }
  
}

// ***********************
// * Poll Config
// ***********************

poll_config_message poll_config_struct(uint8_t *mac) {
  poll_config_message msg;
  msg.msgType = POLL_CONFIG;
  memcpy(&msg.mac, mac, MAC_ADDR_LENGTH);
  return msg;
}

void poll_config(uint8_t *mac){
  
  if (xSemaphoreTake(xMutex_DataPoll, portMAX_DELAY) == pdTRUE) {
    poll_success = false;
    poll_config_message msg = poll_config_struct(mac);
    sendLoraMessage((uint8_t *) &msg, sizeof(msg));
    Serial.printf("Sent config poll message to:");
    printMacAddress(mac);Serial.println();Serial.println();
    waitForPollAck(); // check for ack before proceeding to next one
    xSemaphoreGive(xMutex_DataPoll);
  }

}

// ***********************
// * Time Sync
// ***********************

time_sync_message get_current_time_struct() {
  time_sync_message msg;

  if (!rtc_mounted) {
    Serial.println("External RTC not mounted. Falling back to system time.");

    // Attempt to get the system time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain system time.");
      // Return a message with an invalid type or handle this error as needed
      msg.msgType = 0xFF; // Invalid type for error indication
      return msg;
    }

    msg.msgType = TIME_SYNC;
    msg.pairingKey = systemConfig.PAIRING_KEY; // key for network
    msg.year = timeinfo.tm_year + 1900;
    msg.month = timeinfo.tm_mon + 1;
    msg.day = timeinfo.tm_mday;
    msg.hour = timeinfo.tm_hour;
    msg.minute = timeinfo.tm_min;
    msg.second = timeinfo.tm_sec;

    return msg;
  }

  DateTime now = rtc.now();
  msg.msgType = TIME_SYNC; // Define TIME_SYNC
  msg.pairingKey = systemConfig.PAIRING_KEY; // key for network
  msg.year = now.year();
  msg.month = now.month();
  msg.day = now.day();
  msg.hour = now.hour();
  msg.minute = now.minute();
  msg.second = now.second();

  return msg;
}

void send_time_sync_message() {
  time_sync_message msg = get_current_time_struct();

  if (msg.msgType == 0xFF) {
    // Handle error if time retrieval failed
    return;
  }

  uint8_t buffer[sizeof(time_sync_message)];
  memcpy(buffer, &msg, sizeof(time_sync_message));

  // only execute if not in data transfer mode
  if (xSemaphoreTake(xMutex_DataPoll, portMAX_DELAY) == pdTRUE) {
    sendLoraMessage(buffer, sizeof(time_sync_message));
    xSemaphoreGive(xMutex_DataPoll);
  }

}

/******************************************************************
 *                         Control Tasks                          *
 ******************************************************************/
void gateway_scheduled_poll(void *parameter){

  while(true){

    unsigned long currentTime = millis();

    // Check if it's time to poll data
    if ((currentTime - lastPollTime) >= pollInterval) {
      lastPollTime = currentTime;
      for(int i = 0; i < peerCount; i++){
        poll_data(peers[i].mac);
      }
    }

    // time synchronization (broadcast)
    if ((currentTime - lastTimeSyncTime) >= timeSyncInterval) {
      lastPollTime = currentTime;
      send_time_sync_message();
    }

    // configuration verification
    if ((currentTime - lastConfigSyncTime) >= configSyncInterval) {
      lastConfigSyncTime = currentTime;
      for(int i = 0; i < peerCount; i++){
        poll_config(peers[i].mac);
      }
    }

    // Sleep for a short interval before next check (if needed)
    vTaskDelay(10 / portTICK_PERIOD_MS); // Delay for 1 second
  }
}

/******************************************************************
 *                                                                *
 *                        Initialization                          *
 *                                                                *
 ******************************************************************/

void lora_gateway_init() {

  LoRa.onReceive(onReceive);
  LoRa.receive();

  loadPeersFromSD();
  memset(peer_ack, false, sizeof(peer_ack));

  // Ensure the "data" directory exists
  if (!SD.exists("/node")) {
    Serial.print("Creating 'node' directory - ");
    if (SD.mkdir("/node")) {
      Serial.println("OK");
    } else {
      Serial.println("Failed to create 'node' directory");
    }
  }
  
  Serial.println("Finished checking node dir");

  // Create the task for the receive loop
  xTaskCreate(
    taskReceive,
    "Data Receive Handler",
    10000,
    (void*)OnDataRecvGateway,
    1,
    NULL
  );

  Serial.println("Added Data receieve handler");

  // Create the task for the control loop
  xTaskCreate(
    gateway_scheduled_poll,    // Task function
    "Control Task",     // Name of the task
    10000,              // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the task
    NULL                // Task handle
  );
  Serial.println("Added Data send handler");

  poll_data(peers[0].mac);

  
}