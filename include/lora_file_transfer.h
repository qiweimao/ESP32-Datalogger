#include "lora_init.h"

#define MAX_ATTEMPS 5

// Sender Functions
bool sendFile(const char* filename, int mode = 0);
bool sendMetadata(String filename, size_t fileSize);
bool sendChunk(file_body_message file_body);
bool sendEndOfTransfer();
int waitForAck();

// Receiver Functions
void handle_file_meta(const uint8_t *incomingData);
void handle_file_body(const uint8_t *incomingData);
int handle_file_end(const uint8_t *incomingData);