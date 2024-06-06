#include "lora_init.h"

#define MAX_ATTEMPS 5

bool sendFile(const char* filename);
bool sendMetadata(String filename, size_t fileSize);
bool sendChunk(file_body_message file_body);
bool sendEndOfTransfer();
bool waitForAck();