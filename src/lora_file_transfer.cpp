#include <LoRa.h>
#include <SD.h>
#include "lora_init.h"
#include "lora_file_transfer.h"

file_meta_message file_meta;
file_body_message file_body;
file_end_message file_end;

char filename_in_transfer[MAX_FILENAME_LEN];
char device_in_transfer[MAX_DEVICE_NAME_LEN];

/******************************************************************
 *                             Sender                             *
 ******************************************************************/

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
    delay(500);  // Small delay to avoid congestion
  }

  if(!sendEndOfTransfer()){
    return false;
  }
  file.close();
  Serial.println("File Transfer: SUCCESS");
  return true;
}

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
    Serial.println();
    Serial.println("Sent metadata");

    if (waitForAck()) {
      Serial.println("Received ACK");
      return true;
    } else {
      Serial.println("ACK for metadata not received, resending");
      attempts++;
    }
  }
  Serial.println("Failed to send metadata after maximum attempts");
  return false;
}



bool sendChunk(file_body_message file_body) {
  int attempts = 0;

  while (attempts < MAX_ATTEMPS) {

    sendLoraMessage((uint8_t*)&file_body, sizeof(file_body));
    Serial.print("\nSent chunk of size: ");
    Serial.println(file_body.len);

    if (waitForAck()) {
      return true;
    } else {
      Serial.println("ACK not received, resending chunk");
      attempts++;
    }
  }
  Serial.println("Failed to send chunk after maximum attempts");
  return false;
}


bool sendEndOfTransfer() {
  int attempts = 0;

  file_end.msgType = FILE_END;
  memcpy(file_end.mac, MAC_ADDRESS_STA, sizeof(file_end.mac));

  while (attempts < MAX_ATTEMPS) {
    sendLoraMessage((uint8_t*)&file_end, sizeof(file_end));
    Serial.println("Sent end of transfer");

    if (waitForAck()) {
      return true;
    } else {
      Serial.println("ACK for end of transfer not received, resending");
      attempts++;
    }
  }
  Serial.println("Failed to send end of transfer after maximum attempts");
  return false;
}

bool waitForAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < ACK_TIMEOUT) {
      if(ack_count){
        ack_count--;
        return true;
      }
  }
  return false;
}

void unpack_file_name(file_meta_message* file_meta_gateway, char* output_buffer, size_t buffer_len) {
  if (buffer_len < MAX_FILENAME_LEN + 1) {
    // Handle error: output_buffer is too small
    printf("Error: buffer too small\n");
    return;
  }
  
  strncpy(output_buffer, file_meta_gateway->filename, MAX_FILENAME_LEN);
  output_buffer[MAX_FILENAME_LEN] = '\0'; // Ensure null-termination
}


/******************************************************************
 *                             Receiver                           *
 ******************************************************************/