#include "lora_init.h"

enum LoRaFileTransferMode { SEND, SYNC };

// Sender Functions
bool sendLoRaFile(const char* filename, LoRaFileTransferMode mode = SEND);
bool sendLoRaData(uint8_t *data, size_t size, const char *filename);
bool sendChunk(file_body_message file_body);

// Receiver Functions
void handle_file_body(const uint8_t *incomingData);
void handle_file_entire(const uint8_t *incomingData);