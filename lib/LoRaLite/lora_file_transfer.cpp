#include <LoRa.h>
#include <SD.h>
#include "lora_init.h"
#include "lora_file_transfer.h"

/******************************************************************
 *                             Sender                             *
 ******************************************************************/

// **************************************
// * Check ACK
// **************************************
int waitForAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < ACK_TIMEOUT) {
    // Serial.println("Checking flags");
    if(ack_count){
      // Serial.println("Checked ack_count flag");
      ack_count--;
      return ACK;
    }
    if(rej_count){
      rej_count--;
      // Serial.println("Checked rej_count flag");
      return REJ;
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); // Delay for 1 second
  }
  return TIMEOUT;
}

// **************************************
// * Send Data From RAM
// **************************************
// mode SEND: entire file transfer
// mode SYNC: file synchronization
bool sendLoRaData(uint8_t *data, size_t size, const char *filename) {
  
    file_body_message file_body;
    file_body.msgType = FILE_ENTIRE; // msgType

    memcpy(file_body.mac, MAC_ADDRESS_STA, sizeof(file_body.mac));          // MAC
    memset(file_body.filename, 0, sizeof(file_body.filename));              // filename --> the full file path
    strncpy(file_body.filename, filename, sizeof(file_body.filename) - 1);
    file_body.filename[sizeof(file_body.filename) - 1] = '\0';
    file_body.filesize = size; // filesize

    size_t offset = 0;

    // Pack File Body
    while (offset < size) {

      // Determine the length of data to copy
      file_body.len = (size - offset) < CHUNK_SIZE ? (size - offset) : CHUNK_SIZE;

      // Copy the data chunk into file_body.data
      memcpy(file_body.data, data + offset, file_body.len);

      // Send the data chunk
      if (!sendChunk(file_body)) {
          Serial.println("File Transfer: FAILED");
          return false;
      }

      // Update offset and msgType for the next chunk
      offset += file_body.len;
      file_body.msgType = FILE_BODY;
    }

    Serial.println("File Transfer: SUCCESS");
    return true;
}

// **************************************
// * Send File on SD Card
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

// mode SEND: entire file transfer
// mode SYNC: file synchronization
bool sendLoRaFile(const char* filename, LoRaFileTransferMode mode) {

  Serial.printf("sendLoRaFile %s ", filename);

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
    Serial.println("Meta file loaded.");
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
  int res;
  while ((file_body.len = file.read(file_body.data, CHUNK_SIZE)) > 0) {
    res = sendChunk(file_body);
    if(!res){
      Serial.println("sendLoRaFile: Abort transimission.");
      break;
    }

    // change the msgType back to SYNC to append to first chunk later
    if (mode == SEND) {
      file_body.msgType = FILE_BODY;
    }
    lastSentPosition = file.position(); // Update the current position
    vTaskDelay(1 / portTICK_PERIOD_MS); // Delay for 1 second
  }
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
  metaFile.println(lastSentPosition); // Write the current position to the meta file
  metaFile.close();

  Serial.println("File Transfer: SUCCESS");

  if(!res){
    return false;
  }

  return true;
}

// **************************************
// * Send File Body
// **************************************
bool sendChunk(file_body_message file_body) {
  int attempts = 0;

  while (attempts < MAX_ATTEMPS) {

    ack_count = 0; // reset flag
    rej_count = 0;
    sendLoraMessage((uint8_t*)&file_body, sizeof(file_body));
    Serial.print("Sent chunk of size: "); Serial.println(file_body.len);
    int res = waitForAck();
    if (res == ACK) {
      // Serial.println("Received ACK");
      return true;
    } else {
      if (res == REJ){
        Serial.println("Sendchunk: Received REJ, Abort Transmission");
        return false;
      }
      // Check rejection before reattempt
      Serial.println("Sendchunk: Time out. ACK not received.");
      attempts++;
    }

  }
  // Serial.println("Failed to send chunk after maximum attempts");
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
  Serial.print("handle_file_body Received FILE_BODY for: ");


  if (rej_switch)
  {
    Serial.print("rej_switch is on, sent REJ.");
    signal_message ackMessage_gateway;
    ackMessage_gateway.msgType = REJ;
    memcpy(&ackMessage_gateway.mac, file_body_gateway.mac, sizeof(file_body_gateway.mac));

    if (xSemaphoreTake(xMutex_LoRaHardware, portMAX_DELAY) == pdTRUE) {
      sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));
      xSemaphoreGive(xMutex_LoRaHardware);
      return;
    }
    return;
  }
  

  String filepath = "/node/" + getDeviceNameByMac(file_body_gateway.mac)  + file_body_gateway.filename;
  Serial.print(filepath);
  File file = SD.open(filepath, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  // Write the file_body_gateway structure to the file
  total_bytes_received += file_body_gateway.len;
  bytes_written = file.write((const uint8_t*)&file_body_gateway.data, file_body_gateway.len);
  total_bytes_written += bytes_written;
  Serial.printf(" Wrote %d bytes. ", bytes_written);
  file.close();
  
  signal_message ackMessage_gateway;
  ackMessage_gateway.msgType = ACK;
  memcpy(&ackMessage_gateway.mac, file_body_gateway.mac, sizeof(file_body_gateway.mac));

  if (xSemaphoreTake(xMutex_LoRaHardware, portMAX_DELAY) == pdTRUE) {
    sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));
    xSemaphoreGive(xMutex_LoRaHardware);
  }

  Serial.println("Data written to file successfully");

}
// ***********************
// * Handle File Sync
// ***********************
void handle_file_entire(const uint8_t *incomingData){

  file_body_message file_body_gateway;
  memcpy(&file_body_gateway, incomingData, sizeof(file_body_gateway));
  Serial.print("handle_file_entire Received FILE_ENTIRE for: ");

  String filepath = "/node/" + getDeviceNameByMac(file_body_gateway.mac)  + file_body_gateway.filename;
  Serial.print(filepath);
  File file = SD.open(filepath, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file");
    return;
  }

  // Write the file_body_gateway structure to the file
  total_bytes_received += file_body_gateway.len;
  bytes_written = file.write((const uint8_t*)&file_body_gateway.data, file_body_gateway.len);
  total_bytes_written += bytes_written;
  Serial.printf(" Wrote %d bytes. ", bytes_written);
  file.close();
  Serial.println("Data written to file successfully");
  
  signal_message ackMessage_gateway;
  ackMessage_gateway.msgType = ACK;
  memcpy(&ackMessage_gateway.mac, file_body_gateway.mac, sizeof(file_body_gateway.mac));
  if (xSemaphoreTake(xMutex_LoRaHardware, portMAX_DELAY) == pdTRUE) {
    sendLoraMessage((uint8_t *) &ackMessage_gateway, sizeof(ackMessage_gateway));
    xSemaphoreGive(xMutex_LoRaHardware);
  }
  Serial.println("Sent ACK");

}