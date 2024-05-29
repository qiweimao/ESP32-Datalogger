#include <LoRa.h>
#include <SD.h>
#include "lora_init.h"
#include "lora_file_transfer.h"

file_meta_message file_meta;
file_body_message file_body;
file_end_message file_end;

/******************************************************************
 *                             Sender                             *
 ******************************************************************/

void sendFile(const char* filename) {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file!");
    return;
  }

  size_t fileSize = file.size();
  sendMetadata(filename, fileSize);

  while ((file_body.len = file.read(file_body.data, CHUNK_SIZE)) > 0) {
    sendChunk(file_body);
    delay(500);  // Small delay to avoid congestion
  }

  sendEndOfTransfer();
  file.close();
}

void sendMetadata(String filename, size_t fileSize) {
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

    if (waitForAck()) {
      return;
    } else {
      Serial.println("ACK for metadata not received, resending");
      attempts++;
    }
  }
  Serial.println("Failed to send metadata after maximum attempts");
}



void sendChunk(file_body_message file_body) {
  int attempts = 0;

  while (attempts < MAX_ATTEMPS) {

    sendLoraMessage((uint8_t*)&file_body, sizeof(file_body));
    Serial.print("Sent chunk of size: ");
    Serial.println(file_body.len);

    if (waitForAck()) {
      return;
    } else {
      Serial.println("ACK not received, resending chunk");
      attempts++;
    }
  }
  Serial.println("Failed to send chunk after maximum attempts");
}


void sendEndOfTransfer() {
  int attempts = 0;

  file_end.msgType = FILE_END;
  memcpy(file_end.mac, MAC_ADDRESS_STA, sizeof(file_end.mac));

  while (attempts < MAX_ATTEMPS) {
    sendLoraMessage((uint8_t*)&file_end, sizeof(file_end));
    Serial.println("Sent end of transfer");

    if (waitForAck()) {
      return;
    } else {
      Serial.println("ACK for end of transfer not received, resending");
      attempts++;
    }
  }
  Serial.println("Failed to send end of transfer after maximum attempts");
}

bool waitForAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < ACK_TIMEOUT) {
      ack_count > 0;
      ack_count--;
      return true;
  }
  return false;
}

/******************************************************************
 *                             Receiver                           *
 ******************************************************************/