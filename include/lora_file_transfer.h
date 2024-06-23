#include "lora_init.h"

#define MAX_ATTEMPS 5

enum LoRaFileTransferMode { SEND, SYNC };

// Sender Functions
bool sendFile(const char* filename, LoRaFileTransferMode mode = SEND);
bool sendChunk(file_body_message file_body);

// Receiver Functions
void handle_file_meta(const uint8_t *incomingData);
void handle_file_body(const uint8_t *incomingData);
int handle_file_end(const uint8_t *incomingData);