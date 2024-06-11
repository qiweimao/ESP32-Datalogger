#include "lora_gateway.h"
#include "lora_peer.h"
#include "configuration.h"
#include "utils.h"

struct_message incomingReadings;
struct_pairing pairingDataGateway;
file_meta_message file_meta_gateway;
ack_message ackMessage_gateway;
reject_message reject_message_gateway;
file_body_message file_body_gateway;
String current_file_path;
file_end_message file_end_gateway;
time_sync_message time_sync_gateway;
size_t total_bytes_received;
size_t total_bytes_written;
size_t bytes_written;

File file;

unsigned long lastPollTime = 0;
unsigned long lastTimeSyncTime = 0;
unsigned long lastConfigSyncTime = 0;
const unsigned long pollInterval = 300000; // 5 minute
const unsigned long timeSyncInterval = 86400000; // 24 hours
const unsigned long configSyncInterval = 86400000; // 24 hours

bool apiTriggered = false;

/******************************************************************
 *                                                                *
 *                        Receive Control                         *
 *                                                                *
 ******************************************************************/

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

      oled_print("Pair request: ");
      // oled_print(pairingDataGateway.mac[0]);

      if (pairingDataGateway.msgType == PAIRING) { 

        if(pairingDataGateway.pairingKey == systemConfig.PAIRING_KEY){

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

      memcpy(&file_body_gateway, incomingData, sizeof(file_body_gateway));
      Serial.println("Received FILE_BODY.");

      // If file path is not set
      if(current_file_path == ""){
        Serial.println("Reject file body, no path set.");
        reject_message_gateway.msgType = REJ;
        memcpy(&reject_message_gateway.mac, file_body_gateway.mac, sizeof(file_body_gateway.mac));
        sendLoraMessage((uint8_t *) &reject_message_gateway, sizeof(reject_message_gateway));
      }

      Serial.println(current_file_path);
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
      memcpy(&ackMessage_gateway.mac, file_body_gateway.mac, sizeof(file_meta_gateway.mac));
      sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));

      Serial.println("Data written to file successfully");

      break;


    case FILE_META:

      memcpy(&file_meta_gateway, incomingData, sizeof(file_meta_gateway));

      // Reject new transmission, if already in transmission
      if(current_file_path != ""){
        Serial.println("Already in transfer mode, Reject New file transfer");
        reject_message_gateway.msgType = REJ;
        memcpy(&reject_message_gateway.mac, file_meta_gateway.mac, sizeof(file_meta_gateway.mac));
        sendLoraMessage((uint8_t *) &reject_message_gateway, sizeof(reject_message_gateway));
        return;
      }

      // Get File Name
      char buffer[MAX_FILENAME_LEN + 1]; // 10 for the name + 1 for the null terminator
      strncpy(buffer, file_meta_gateway.filename, MAX_FILENAME_LEN);
      buffer[MAX_FILENAME_LEN] = '\0'; // Ensure null-termination
      char filename[MAX_FILENAME_LEN + 2]; // 1 for '/' + 4 for 'data' + 1 for '/' + 10 for the name + 1 for the null terminator
      snprintf(filename, sizeof(filename), "%s", buffer);

      Serial.println("Recieved FILE_META");
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

      /* Need to verify if node has the permission to end file transmission */
      // Add if encounters collision, current one to many design assumes node
      // would not reach this point before getting rejected
      /* Need to verify if node has the permission to end file transmission */

      memcpy(&file_end_gateway, incomingData, sizeof(file_end_gateway));
      Serial.print("\nReceived FILE_END from:");
      printMacAddress(file_end_gateway.mac); Serial.println();
      Serial.print("Total bytes received: "); Serial.println(total_bytes_received);
      current_file_path = "";

      ackMessage_gateway.msgType = ACK;
      memcpy(&ackMessage_gateway.mac, file_end_gateway.mac, sizeof(ackMessage_gateway.mac));
      sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));

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

// ***********************
// * Time Sync
// ***********************

time_sync_message get_current_time_struct() {
  time_sync_message msg;

  if(!rtc_mounted){
    Serial.println("External RTC not mounted. Cannot get current time.");
    // Return a message with an invalid type or handle this error as needed
    msg.msgType = 0xFF; // Invalid type for error indication
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
  unsigned long currentTime = millis();

  // Check if it's time to poll data
  if ((currentTime - lastPollTime) >= pollInterval) {
    lastPollTime = currentTime;
    // Poll data from slaves
    // Your polling code here
  }

  // Check if the API triggered flag is set
  if (apiTriggered) {
    apiTriggered = false;
    // Handle Web API commands
    // Your Web API command handling code here
  }

  // Check if it's time to perform time synchronization
  if ((currentTime - lastTimeSyncTime) >= timeSyncInterval) {
    lastPollTime = currentTime;
    send_time_sync_message();
  }

  // Check if it's time to perform configuration synchronization
  if ((currentTime - lastConfigSyncTime) >= configSyncInterval) {
    lastConfigSyncTime = currentTime;
    // Perform configuration synchronization
    // Your config sync code here
  }

  // Sleep for a short interval before next check (if needed)
  delay(100);
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

  // Ensure the "data" directory exists
  if (!SD.exists("/node")) {
    Serial.print("Creating 'node' directory - ");
    if (SD.mkdir("/node")) {
      Serial.println("OK");
    } else {
      Serial.println("Failed to create 'node' directory");
    }
  }
  
  xTaskCreate(taskReceive, "Data Receive Handler", 10000, (void*)OnDataRecvGateway, 1, NULL);
  // Create the task for the control loop
  xTaskCreate(
    gateway_send_control,    // Task function
    "Control Task",     // Name of the task
    10000,              // Stack size in words
    NULL,               // Task input parameter
    1,                  // Priority of the task
    NULL                // Task handle
  );
}