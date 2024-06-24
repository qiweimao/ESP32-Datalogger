#include <LoRa.h>
#include <SD.h>
#include "lora_init.h"
#include "lora_file_transfer.h"
#include "utils.h"

/******************************************************************
 *                             Sender                             *
 ******************************************************************/

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
        vTaskDelay(1 / portTICK_PERIOD_MS); // Delay for 1 second
  }
  return TIMEOUT;
}

// **************************************
// * Wrapper Function for Sending File
// **************************************

// Get Current Position from Meta File
String getMetaFilename(const char* filename) {
  String filenameStr = String(filename);
  int dotIndex = filenameStr.lastIndexOf('.');
  if (dotIndex > 0) {
    filenameStr = filenameStr.substring(0, dotIndex);
  }
  return filenameStr + ".meta";
}

// mode 0 is entire file transfer, mode 1 id for file synchronization
bool sendFile(const char* filename, LoRaFileTransferMode mode) {

  size_t lastSentPosition = 0;
  if (mode == SYNC) {
    // File Synchronization
    String metaFilename = getMetaFilename(filename);
    File metaFile = SD.open(metaFilename.c_str(), FILE_READ);
    if (!metaFile) {
      Serial.println("Meta file does not exist. Creating new meta file.");
      metaFile = SD.open(metaFilename.c_str(), FILE_WRITE);
      if (!metaFile) {
        Serial.println("Failed to create meta file!");
        return false;
      }
      metaFile.println(0); // Write initial position 0 to the meta file
    } else {
      // Read the last sent position from the .meta file
      lastSentPosition = metaFile.parseInt();
    }
    metaFile.close();
  }

  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file!");
    return false;
  }

  file_body_message file_body;

  // Pack Meta Data
  if (mode == SYNC) {
    file_body.msgType = FILE_BODY;                                            // msgType
  }
  else{
    file_body.msgType = FILE_ENTIRE;                                            // msgType
  }
  memcpy(file_body.mac, MAC_ADDRESS_STA, sizeof(file_body.mac));            // MAC
  memset(file_body.filename, 0, sizeof(file_body.filename));                // filename --> the full file path
  strncpy(file_body.filename, filename, sizeof(file_body.filename) - 1);
  file_body.filename[sizeof(file_body.filename) - 1] = '\0';
  size_t fileSize = file.size();
  file_body.filesize = fileSize;                                            // filesize

  // Pack File Body
  file.seek(lastSentPosition);// Seek to the last sent position in the data file
  while ((file_body.len = file.read(file_body.data, CHUNK_SIZE)) > 0) {
    if(!sendChunk(file_body)){
      return false;
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); // Delay for 1 second
  }
  int currentPosition = file.position(); // Update the current position
  file.close();

  if (mode == SEND) { return true;}

  // Update the meta file with the last sent position
  String metaFilename = getMetaFilename(filename);
  File metaFile = SD.open(metaFilename.c_str(), FILE_WRITE);
  if (!metaFile) {
    Serial.println("Failed to open meta file for updating!");
    return false;
  }
  metaFile.seek(0);
  metaFile.println(currentPosition); // Write the current position to the meta file
  metaFile.close();

  Serial.println("File Transfer: SUCCESS");
  return true;
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

/******************************************************************
 *                             Receiver                           *
 ******************************************************************/

size_t total_bytes_received;
size_t total_bytes_written;
size_t bytes_written;

// ***********************
// * Handle File Body
// ***********************
void handle_file_body(const uint8_t *incomingData){

  file_body_message file_body_gateway;
  memcpy(&file_body_gateway, incomingData, sizeof(file_body_gateway));
  Serial.println("Received FILE_BODY.");

  String filepath = "/node/" + getDeviceNameByMac(file_body_gateway.mac)  + file_body_gateway.filename;
  Serial.println(filepath);
  File file = SD.open(filepath, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  // Write the file_body_gateway structure to the file
  total_bytes_received += file_body_gateway.len;
  bytes_written = file.write((const uint8_t*)&file_body_gateway.data, file_body_gateway.len);
  total_bytes_written += bytes_written;
  Serial.printf("Wrote %d bytes\n", bytes_written);
  file.close();
  
  signal_message ackMessage_gateway;
  ackMessage_gateway.msgType = ACK;
  memcpy(&ackMessage_gateway.mac, file_body_gateway.mac, sizeof(file_body_gateway.mac));
  sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));

  Serial.println("Data written to file successfully");

}
// ***********************
// * Handle File Sync
// ***********************
void handle_file_entire(const uint8_t *incomingData){

  file_body_message file_body_gateway;
  memcpy(&file_body_gateway, incomingData, sizeof(file_body_gateway));
  Serial.println("Received FILE_BODY.");

  String filepath = "/node/" + getDeviceNameByMac(file_body_gateway.mac)  + file_body_gateway.filename;
  Serial.println(filepath);
  File file = SD.open(filepath, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  // Write the file_body_gateway structure to the file
  total_bytes_received += file_body_gateway.len;
  bytes_written = file.write((const uint8_t*)&file_body_gateway.data, file_body_gateway.len);
  total_bytes_written += bytes_written;
  Serial.printf("Wrote %d bytes\n", bytes_written);
  file.close();
  
  signal_message ackMessage_gateway;
  ackMessage_gateway.msgType = ACK;
  memcpy(&ackMessage_gateway.mac, file_body_gateway.mac, sizeof(file_body_gateway.mac));
  sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));

  Serial.println("Data written to file successfully");

}