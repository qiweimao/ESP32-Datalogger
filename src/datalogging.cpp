#include <FS.h>
#include <SPIFFS.h>
#include "utils.h"
#include "datalogging.h"

// extern const char *filename;
// extern const int LOG_INTERVAL;

void logFlashSize() {
  Serial.println("Logging flash size.");
  // Open file in append mode
  File file = SPIFFS.open(filename, "a");
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  Serial.println("Getting current time.");
  String formattedTime = getCurrentTime();

  // Get FLash Space
  size_t fileSize = file.size();
  size_t spiffsTotal = SPIFFS.totalBytes();
  size_t spiffsUsed = SPIFFS.usedBytes();

  if(!file.println(String(formattedTime) + "," + String(fileSize) + "," + spiffsUsed + "," + spiffsTotal)){
    Serial.println("Error Writing file");
  }
    
  file.close();
  delay(LOG_INTERVAL);
  Serial.println("Finished Logging");
}