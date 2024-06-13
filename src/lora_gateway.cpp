#include "lora_gateway.h"
#include "lora_file_transfer.h"
#include "lora_peer.h"
#include "configuration.h"
#include "utils.h"

unsigned long lastPollTime = 0;
unsigned long lastTimeSyncTime = 0;
unsigned long lastConfigSyncTime = 0;
const unsigned long pollInterval = 60000; // 1 minute
const unsigned long timeSyncInterval = 86400000; // 24 hours
const unsigned long configSyncInterval = 86400000; // 24 hours

bool apiTriggered = false;
bool peer_ack[MAX_PEERS];
bool poll_data_success = false;

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
        poll_data_success = true;
      }
      break;
    default:
      Serial.print("This message is not for me.");
  }
}

/******************************************************************
 *                                                                *
 *                         Send Control                           *
 *                                                                *
 ******************************************************************/

// Room for improvement. lower the while loop check frequency
int waitForPollDataAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < ACK_TIMEOUT) {
    if(poll_data_success){
      poll_data_success = false;
      return true;
    }
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

void send_pool_data_message(uint8_t *mac){
  Serial.println("Sending data poll message");
  printMacAddress(mac);
  poll_data_message msg = poll_data_struct(mac);
  sendLoraMessage((uint8_t *) &msg, sizeof(msg));
  waitForPollDataAck(); // check for ack before proceeding to next one
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
  sendLoraMessage(buffer, sizeof(time_sync_message));
}

// ***********************
// * Control Loop
// ***********************

// Control Callback
void gateway_send_control(void *parameter){
  /* May be I should set the ack array here to initialize? */
  while(true){
    unsigned long currentTime = millis();

    // Check if it's time to poll data
    if ((currentTime - lastPollTime) >= pollInterval) {
      lastPollTime = currentTime;
      for(int i = 0; i < peerCount; i++){
        poll_data_success = false;
        send_pool_data_message(peers[i].mac);
      }
    }

    // Check if the API triggered flag is set
    if (apiTriggered) {
      apiTriggered = false;
      // Handle Web API commands
      // Your Web API command handling code here
    }

    // time synchronization
    if ((currentTime - lastTimeSyncTime) >= timeSyncInterval) {
      lastPollTime = currentTime;
      send_time_sync_message();
    }

    // configuration verification
    if ((currentTime - lastConfigSyncTime) >= configSyncInterval) {
      lastConfigSyncTime = currentTime;
      // Perform configuration synchronization
      // Your config sync code here
    }

    // Sleep for a short interval before next check (if needed)
    delay(100);
  }
}

/******************************************************************
 *                                                                *
 *                        Initialization                          *
 *                                                                *
 ******************************************************************/

void lora_gateway_init() {

  LoRa.onReceive(onReceive);
  LoRa.onTxDone(onTxDone);
  LoRa_rxMode();

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
    gateway_send_control,    // Task function
    "Control Task",     // Name of the task
    10000,              // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the task
    NULL                // Task handle
  );
  Serial.println("Added Data send handler");
  
}