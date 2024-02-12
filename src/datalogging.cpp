#include <FS.h>
#include <SPIFFS.h>
#include "utils.h"
#include "datalogging.h"

LogErrorCode logFlashSize() {
  File file;
  if (!SD.exists(filename)) {
    Serial.println("File does not exist");
    const char* message = "Time, Used SD (KB), SD Total (KB), SD Used (%), FreeRAM, TotalRAM, maxAllocatableRAM\n";
    writeFile(SD, filename, message);
  }

  // Get information about RAM
  size_t freeRAM = ESP.getFreeHeap();
  size_t totalRAM = ESP.getHeapSize();
  size_t maxAllocatableRAM = ESP.getMaxAllocHeap();
  size_t totalSpace = SD.totalBytes();
  size_t usedSpace = SD.usedBytes();
  float percentUsed = (float)usedSpace / totalSpace * 100.0;

  String formattedTime = getCurrentTime();

  String data = String(formattedTime) + "," + String(usedSpace) + "," + String(totalSpace) + "," + String(percentUsed) + "%," +
                String(freeRAM) + "," + String(totalRAM) + "," + String(maxAllocatableRAM) + "\n";
  const char* data_char = data.c_str();
  // Serial.printf("%s", data_char);
  // TelnetStream.printf("%s", data_char);
  appendFile(SD, filename, data_char);

  delay(LOG_INTERVAL);
  return LOG_SUCCESS;
}