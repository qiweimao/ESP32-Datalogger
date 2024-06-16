#include <LoRa.h>
#include <SD.h>
#include "lora_init.h"
#include "lora_file_transfer.h"
#include "utils.h"


/******************************************************************
 *                             Sender                             *
 ******************************************************************/

file_meta_message file_meta;
file_body_message file_body;
file_end_message file_end;

// **************************************
// * Wrapper Function for Sending File
// **************************************

bool sendFile(const char* filename) {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file!");
    return false;
  }
  size_t fileSize = file.size();
  if(!sendMetadata(filename, fileSize)){
    return false;
  }

  while ((file_body.len = file.read(file_body.data, CHUNK_SIZE)) > 0) {
    file_body.msgType = FILE_BODY;
    memcpy(file_body.mac, MAC_ADDRESS_STA, sizeof(file_meta.mac));
    if(!sendChunk(file_body)){
      return false;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS); // Delay for 1 second
  }

  if(!sendEndOfTransfer()){
    return false;
  }
  file.close();
  Serial.println("File Transfer: SUCCESS");
  return true;
}

// **************************************
// * Send File Meta Data
// **************************************
bool sendMetadata(String filename, size_t fileSize) {
  file_meta_message file_meta;
  file_meta.msgType = FILE_META;
  memcpy(file_meta.mac, MAC_ADDRESS_STA, sizeof(file_meta.mac));
  memset(file_meta.filename, 0, sizeof(file_meta.filename));
  strncpy(file_meta.filename, filename.c_str(), sizeof(file_meta.filename) - 1);
  file_meta.filename[sizeof(file_meta.filename) - 1] = '\0';
  file_meta.filesize = fileSize;

  int attempts = 0;

  while (attempts < MAX_ATTEMPS) {

    sendLoraMessage((uint8_t*)&file_meta, sizeof(file_meta));
    Serial.println("Sent metadata");

    int res = waitForAck();
    if (res == ACK) {
      Serial.println("Received ACK");
      return true;
    } else {
      if (res == REJ){
        Serial.println("Received REJ, Abort Transmission");
        return false;
      }
      // Check rejection before reattempt
      Serial.println("Time out. ACK for metadata not received, resending");
      attempts++;
    }

  }
  Serial.println("Failed to send metadata after maximum attempts");
  return false;
}

// **************************************
// * Send File Body
// **************************************
bool sendChunk(file_body_message file_body) {
  int attempts = 0;

  while (attempts < MAX_ATTEMPS) {

    sendLoraMessage((uint8_t*)&file_body, sizeof(file_body));
    Serial.print("Sent FILE_BODY, chunk of size: ");
    Serial.println(file_body.len);

    int res = waitForAck();
    if (res == ACK) {
      Serial.println("Received ACK");
      return true;
    } else {
      if (res == REJ){
        Serial.println("Received REJ, Abort Transmission");
        return false;
      }
      // Check rejection before reattempt
      Serial.println("Time out. ACK for metadata not received, resending");
      attempts++;
    }

  }
  Serial.println("Failed to send chunk after maximum attempts");
  return false;
}

// **************************************
// * Send EOF
// **************************************
bool sendEndOfTransfer() {
  int attempts = 0;

  file_end.msgType = FILE_END;
  memcpy(file_end.mac, MAC_ADDRESS_STA, sizeof(file_end.mac));

  while (attempts < MAX_ATTEMPS) {
    sendLoraMessage((uint8_t*)&file_end, sizeof(file_end));
    Serial.println("Sent end of transfer");

    int res = waitForAck();
    if (res == ACK) {
      Serial.println("Received ACK");
      return true;
    } else {
      if (res == REJ){
        Serial.println("Received REJ, Abort Transmission");
        return false;
      }
      // Check rejection before reattempt
      Serial.println("Time out. ACK for metadata not received, resending");
      attempts++;
    }

  }
  Serial.println("Failed to send end of transfer after maximum attempts");
  return false;
}

// **************************************
// * Check ACK
// **************************************
int waitForAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < ACK_TIMEOUT) {
      if(ack_count){
        ack_count--;
        return ACK;
      }
      if(rej_count){
        rej_count--;
        return REJ;
      }
        vTaskDelay(10 / portTICK_PERIOD_MS); // Delay for 1 second
  }
  return TIMEOUT;
}

/******************************************************************
 *                             Receiver                           *
 ******************************************************************/

String current_file_path;
size_t total_bytes_received;
size_t total_bytes_written;
size_t bytes_written;
file_meta_message file_meta_gateway;

// ***********************
// * Handle File Meta
// ***********************
void handle_file_meta(const uint8_t *incomingData){
  memcpy(&file_meta_gateway, incomingData, sizeof(file_meta_gateway));

  // Reject new transmission, if already in transmission
  // if(current_file_path != ""){
  //   Serial.println("Already in transfer mode, Reject New file transfer");
  //   Serial.println("The current file path is not cleared:");
  //   Serial.println(current_file_path);
  //   reject_message reject_message_gateway;
  //   reject_message_gateway.msgType = REJ;
  //   memcpy(&reject_message_gateway.mac, file_meta_gateway.mac, sizeof(file_meta_gateway.mac));
  //   sendLoraMessage((uint8_t *) &reject_message_gateway, sizeof(reject_message_gateway));
  //   return;
  // }

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
  File file = SD.open(current_file_path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }
  
  Serial.println("File created successfully");

  // Close the file
  file.close();
  Serial.println("File closed successfully");
  
  ack_message ackMessage_gateway;
  ackMessage_gateway.msgType = ACK;
  memcpy(&ackMessage_gateway.mac, file_meta_gateway.mac, sizeof(ackMessage_gateway.mac));
  sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));
  Serial.println("Sent Meta ACK message successfully");

  total_bytes_received = 0;
}


// ***********************
// * Handle File Body
// ***********************
void handle_file_body(const uint8_t *incomingData){
  file_body_message file_body_gateway;
  memcpy(&file_body_gateway, incomingData, sizeof(file_body_gateway));
  Serial.println("Received FILE_BODY.");

  // If file path is not set
  if(current_file_path == ""){
    Serial.println("Reject file body, no path set.");
    reject_message reject_message_gateway;
    reject_message_gateway.msgType = REJ;
    memcpy(&reject_message_gateway.mac, file_body_gateway.mac, sizeof(file_body_gateway.mac));
    sendLoraMessage((uint8_t *) &reject_message_gateway, sizeof(reject_message_gateway));
  }

  Serial.println(current_file_path);
  File file = SD.open(current_file_path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  // Write the file_body_gateway structure to the file
  // Serial.printf("Received %d bytes\n", file_body_gateway.len);
  total_bytes_received += file_body_gateway.len;

  bytes_written = file.write((const uint8_t*)&file_body_gateway.data, file_body_gateway.len);
  total_bytes_written += bytes_written;
  Serial.printf("Wrote %d bytes\n", bytes_written);
  
  file.close();
  
  ack_message ackMessage_gateway;
  ackMessage_gateway.msgType = ACK;
  memcpy(&ackMessage_gateway.mac, file_body_gateway.mac, sizeof(file_meta_gateway.mac));
  sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));

  Serial.println("Data written to file successfully");

}

// ***********************
// * Handle File End
// ***********************
int handle_file_end(const uint8_t *incomingData){
  /* Need to verify if node has the permission to end file transmission */
  // Add if encounters collision, current one to many design assumes node
  // would not reach this point before getting rejected
  /* Need to verify if node has the permission to end file transmission */
  file_end_message file_end_gateway;
  memcpy(&file_end_gateway, incomingData, sizeof(file_end_gateway));
  Serial.print("Received FILE_END from:");
  printMacAddress(file_end_gateway.mac); Serial.println();
  Serial.print("Total bytes received: "); Serial.println(total_bytes_received);Serial.println();
  current_file_path = "";

  ack_message ackMessage_gateway;
  ackMessage_gateway.msgType = ACK;
  memcpy(&ackMessage_gateway.mac, file_end_gateway.mac, sizeof(ackMessage_gateway.mac));
  sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));

  return 0; // Indicate success
}