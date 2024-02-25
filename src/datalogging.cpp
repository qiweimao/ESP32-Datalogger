#include <FS.h>
#include <SPIFFS.h>
#include "utils.h"
#include "VM_501.h"
#include "datalogging.h"

LogErrorCode logData() {
  File file;
  if (!SD.exists(filename)) {
    Serial.println("File does not exist");
    const char* message = "Time, Frequency (Hz)\n";
    writeFile(SD, filename, message);
  }

  String data = readVM();
  String formattedTime = getCurrentTime();

  String row = String(formattedTime) + "," + String(data);//+ "\n";
  const char* row_char = row.c_str();
  Serial.printf("%s", row_char);

  TelnetStream.printf("%s", row_char);
  appendFile(SD, filename, row_char);

  delay(LOG_INTERVAL);
  return LOG_SUCCESS;
}