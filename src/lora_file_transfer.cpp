#include <LoRa.h>
#include <SD.h>
#include "lora_init.h"
#include "lora_file_transfer.h"

file_meta_message file_meta;
file_body_message file_body;

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
  
  // Initialize the filename buffer to zero
  memset(file_meta.filename, 0, sizeof(file_meta.filename));
  
  // Copy the filename into the buffer
  strncpy(file_meta.filename, filename.c_str(), sizeof(file_meta.filename) - 1);
  
  // Ensure null-termination (strncpy does not null-terminate if the source is too long)
  file_meta.filename[sizeof(file_meta.filename) - 1] = '\0';
  
  file_meta.filesize = fileSize;

  LoRa.beginPacket();
  LoRa.write((uint8_t *) &file_meta, sizeof(file_meta));
  LoRa.endPacket();
  LoRa.receive();
  Serial.println("Sent metadata");

  if (!waitForAck()) {
    Serial.println("ACK for metadata not received, resending");
    sendMetadata(filename, fileSize);  // Resend metadata if ACK not received
  }
}


void sendChunk(file_body_message file_body) {
  LoRa.beginPacket();
  LoRa.write((uint8_t *) &file_body, sizeof(file_body));
  LoRa.endPacket();
  LoRa.receive();
  Serial.print("Sent chunk of size: ");
  Serial.println(file_body.len);

  if (!waitForAck()) {
    Serial.println("ACK not received, resending chunk");
    sendChunk(file_body);  // Resend the chunk if ACK not received
  }
}

void sendEndOfTransfer() {
  LoRa.beginPacket();
  LoRa.print("END");
  LoRa.endPacket();
  Serial.println("Sent end of transfer");

  if (!waitForAck()) {
    Serial.println("ACK for end of transfer not received, resending");
    sendEndOfTransfer();  // Resend end of transfer if ACK not received
  }
}

bool waitForAck() {
  unsigned long startTime = millis();
  while (millis() - startTime < ACK_TIMEOUT) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String ack = "";
      while (LoRa.available()) {
        ack += (char)LoRa.read();
      }
      if (ack == "ACK") {
        Serial.println("ACK received");
        return true;
      }
    }
  }
  return false;
}