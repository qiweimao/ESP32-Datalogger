#include "lora_init.h"

void sendFile(const char* filename);void sendFile(const char* filename);
void sendMetadata(String filename, size_t fileSize);
void sendChunk(file_body_message file_body);
void sendEndOfTransfer();
bool waitForAck();